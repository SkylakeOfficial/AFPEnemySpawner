// Skylake Game Studio


#include "AFPEnemySpawnerActor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"



DEFINE_LOG_CATEGORY(AFPEnemySpawner);

// Sets default values
AAFPEnemySpawnerActor::AAFPEnemySpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root Comp"));
	SetRootComponent(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UTexture2D> BillboardTex(TEXT("/AFPEnemySpawner/Tx_Billboard"));
	if (BillboardTex.Succeeded())
	{
		SceneRoot->Sprite = BillboardTex.Object;
	}
}

void AAFPEnemySpawnerActor::BeginPlay()
{
	Super::BeginPlay();
	if (!SpawnProfileList.IsEmpty())
	{
		SpawnProfile = SpawnProfileList[UKismetMathLibrary::RandomInteger(SpawnProfileList.Num())];
	}
	if (bAutoStart) StartSpawning(); 
} 

void AAFPEnemySpawnerActor::StartSpawning()
{
	if (SpawnStarted||!SpawnProfile) return;
	SpawnStarted = true;

	EEnemySpawnMode SpawnMode = SpawnProfile->SpawnMode;
	switch (SpawnMode){
	case EEnemySpawnMode::AutoStreak:
		Spawn_AutoStart();
		break;
	case EEnemySpawnMode::MaxCount:
	{
		if (SpawnProfile->MaxCount<1)
		{
			UE_LOG(AFPEnemySpawner, Warning, TEXT("Max enemy count illegal, spawn aborted!"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Max enemy count illegal, spawn aborted!"));
			return;
		}
		for (int i = 0; i < SpawnProfile->Streaks.Num(); i++)
		{
			StreakConverted.Append(ConvertSpawnConfigs(SpawnProfile->Streaks[i].EnemyStreak));
		}
		Spawn_MaxCount();
	}
		break;
	case EEnemySpawnMode::LoopInTime:
		{
			if (SpawnProfile->Duration<= 0)
			{
				UE_LOG(AFPEnemySpawner, Warning, TEXT("Battle duration illegal, spawn aborted!"));
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Battle duration illegal, spawn aborted!"));
				return;
			}
			Spawn_LoopInTime();
			GetWorld()->GetTimerManager().SetTimer(TimeLimitTimer, this, &AAFPEnemySpawnerActor::Spawn_LoopInTime_TimeUp, SpawnProfile->Duration, false);

		}
		break;
	default:
		break;
	}
}

//返回战斗名
FText AAFPEnemySpawnerActor::GetBattleName()
{
	if(!SpawnProfile)return FText();
	else {
		return SpawnProfile->BattleName;
	}
}
//返回存活的敌人
TArray<AActor*> AAFPEnemySpawnerActor::GetSurvivingEnemies()
{
	return ExistingEnemies;
}
//返回剩余时间
float AAFPEnemySpawnerActor::GetRemainingTime()
{
	if (SpawnProfile->SpawnMode == EEnemySpawnMode::LoopInTime)
	{
		return GetWorldTimerManager().GetTimerRemaining(TimeLimitTimer);
	}
	return -1.0f;
}


//构造函数
void AAFPEnemySpawnerActor::OnConstruction(const FTransform& Transform)
{

}

//将DataAsset转换为更便于生成的格式
TArray<FSpawnConfig_Converted> AAFPEnemySpawnerActor::ConvertSpawnConfigs(TArray<FEnemySubStreak> InConfig)
{
	TArray<FSpawnConfig_Converted> Result;
	if (InConfig.IsEmpty())
	{
		return Result;
	}
	for (int i = 0; i != InConfig.Num(); ++i) {
		FSpawnConfig_Converted Config;
		//Config.EnemyBT = InConfig[i].EnemyBT;
		Config.EnemyType = InConfig[i].EnemyType;
		FVector Scale = InConfig[i].EnemyTransform.GetScale3D().operator*(FVector(1.0,1.0,0.0));
		FVector Loc = InConfig[i].EnemyTransform.GetLocation();
		FRotator Rot = FRotator(InConfig[i].EnemyTransform.GetRotation());
		for (int j = 0; j < InConfig[i].EnemyCount; ++j) {
			FVector2D OffsetXY = SobolVec2D(j);//生成二维sobol序列
			FVector LocOffset = InConfig[i].EnemyCount == 1 ? FVector(0, 0, 0) : UKismetMathLibrary::MakeVector(OffsetXY.X-0.5f,OffsetXY.Y-0.5f,0)*Scale*80;
			FVector Location = Loc.operator+(LocOffset);
			FTransform SelfTransform = GetTransform();
			SelfTransform.SetScale3D(FVector(1, 1, 1));
			Config.EnemyTransform = UKismetMathLibrary::MakeTransform(Location, Rot).operator+(SelfTransform);
			Result.Emplace(Config);
		}
	}
	return Result;
}


//自动开始模式
void AAFPEnemySpawnerActor::Spawn_AutoStart()
{
	const TArray<FEnemyStreak> Streaks = SpawnProfile->Streaks;
	StreakConverted = ConvertSpawnConfigs(Streaks[CurrentStreak].EnemyStreak);
	float DistrTime = Streaks[CurrentStreak].DistributeTime;
	float StreakDelay = Streaks[CurrentStreak].StreakDelay;
	if (DistrTime<=0)
	{
		DistrTime = 0.1F;
		UE_LOG(AFPEnemySpawner, Warning, TEXT("DistributeTime illegal, Spawner will use the default value"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("DistributeTime Illegal, Spawner will use the default value"));
	}
	if (StreakDelay<0)
	{
		StreakDelay = 1;
		UE_LOG(AFPEnemySpawner, Warning, TEXT("StreakDelay illegal, Spawner will use the default value"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("StreakDelay Illegal, Spawner will use the default value"));
	}
	GetWorldTimerManager().SetTimer(EnemySpawnTimer, this, &AAFPEnemySpawnerActor::Spawn_AutoStart_Inner, Streaks[CurrentStreak].DistributeTime/ (StreakConverted.Num()-1), true, Streaks[CurrentStreak].StreakDelay);

}

//自动开始模式_Inner
void AAFPEnemySpawnerActor::Spawn_AutoStart_Inner()
{
	if (!StreakConverted.IsEmpty())
	{
		Spawn(StreakConverted[0]);
		StreakConverted.RemoveAt(0);
	}
	if (StreakConverted.IsEmpty())
	{
		CurrentStreak += 1;
		GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
		if (CurrentStreak >= SpawnProfile->Streaks.Num())
		{
			OnSpawnCompleted.Broadcast();
			return;
		}
		return;
	}
}
//最大数量模式
void AAFPEnemySpawnerActor::Spawn_MaxCount()
{
	const TArray<FEnemyStreak> Streaks = SpawnProfile->Streaks;

	GetWorldTimerManager().SetTimer(EnemySpawnTimer, this, &AAFPEnemySpawnerActor::Spawn_MaxCount_Inner, 0.1F, true, 0.1F);

}
//最大数量模式_Inner
void AAFPEnemySpawnerActor::Spawn_MaxCount_Inner()
{
	if (StreakConverted.IsEmpty())
	{
		OnSpawnCompleted.Broadcast();
		GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
		return;
	}
	if (ExistingEnemies.Num()< SpawnProfile->MaxCount)
	{
		Spawn(StreakConverted[0]);
		StreakConverted.RemoveAt(0);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
		return;
	}
}
//限时模式
void AAFPEnemySpawnerActor::Spawn_LoopInTime()
{
	const TArray<FEnemyStreak> Streaks = SpawnProfile->Streaks;
	StreakConverted = ConvertSpawnConfigs(Streaks[CurrentStreak].EnemyStreak);
	float DistrTime = Streaks[CurrentStreak].DistributeTime;
	float StreakDelay = Streaks[CurrentStreak].StreakDelay;
	if (DistrTime<=0)
	{
		DistrTime = 0.1F;
		UE_LOG(AFPEnemySpawner, Warning, TEXT("DistributeTime illegal, Spawner will use the default value"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("DistributeTime Illegal, Spawner will use the default value"));
	}
	if (StreakDelay<0)
	{
		StreakDelay = 1;
		UE_LOG(AFPEnemySpawner, Warning, TEXT("StreakDelay illegal, Spawner will use the default value"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("StreakDelay Illegal, Spawner will use the default value"));
	}
	GetWorldTimerManager().SetTimer(EnemySpawnTimer, this, &AAFPEnemySpawnerActor::Spawn_LoopInTime_Inner, Streaks[CurrentStreak].DistributeTime/ (StreakConverted.Num()-1), true, Streaks[CurrentStreak].StreakDelay);
}
//限时模式_Inner
void AAFPEnemySpawnerActor::Spawn_LoopInTime_Inner()
{
	if (!StreakConverted.IsEmpty())
	{
		Spawn(StreakConverted[0]);
		StreakConverted.RemoveAt(0);
	}
	if (StreakConverted.IsEmpty())
	{
		CurrentStreak = (CurrentStreak + 1) % SpawnProfile->Streaks.Num();
		GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
		if (CurrentStreak >= SpawnProfile->Streaks.Num())
		{
			OnSpawnCompleted.Broadcast();
			return;
		}
		return;
	}
}
//限时模式_计时结束调用
void AAFPEnemySpawnerActor::Spawn_LoopInTime_TimeUp()
{
	GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
	OnSpawnCompleted.Broadcast();
	CurrentStreak = SpawnProfile->Streaks.Num();
}

//生成Pawn
void AAFPEnemySpawnerActor::Spawn(FSpawnConfig_Converted SpawnConfig)
{
	//位置
	FVector SpawnLoc = SpawnConfig.EnemyTransform.GetLocation();
	FRotator SpawnRot = FRotator(SpawnConfig.EnemyTransform.GetRotation());

	//检测地面
	const TArray<TEnumAsByte<EObjectTypeQuery>> TraceQueryType = { EObjectTypeQuery::ObjectTypeQuery1, EObjectTypeQuery::ObjectTypeQuery2 };
	FHitResult Result = FHitResult();
	TArray<AActor*> ActorsToIgnore;
	UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), SpawnLoc, SpawnLoc.operator-(FVector(0,0,10000.0)), TraceQueryType,false, ActorsToIgnore,EDrawDebugTrace::None,Result, true);

	//相关config
	//UBehaviorTree* EnemyBT = SpawnConfig.EnemyBT;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//生成
	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(SpawnConfig.EnemyType, SpawnLoc, SpawnRot, SpawnParams);
	//吸附到地面
	FVector ActorBounds;
	SpawnedPawn->GetActorBounds(true, SpawnLoc, ActorBounds, false);
	FVector DestLocation = Result.Location.operator+(FVector(0, 0, ActorBounds.Z));
	//尝试将生成位置吸附到最近导航点
	/*UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
	FVector OutNavLocation;
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		ANavigationData* NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
		if(UNavigationSystemV1::K2_ProjectPointToNavigation(this, DestLocation, OutNavLocation, NavData, UNavigationQueryFilter::StaticClass(), FVector(200)))
		{
			DestLocation = OutNavLocation;
		}
	}*/

	SpawnedPawn->SetActorLocation(DestLocation,true, nullptr, ETeleportType::TeleportPhysics);
	//绑定到actor摧毁的委托
	SpawnedPawn->OnDestroyed.AddDynamic(this, &AAFPEnemySpawnerActor::OnEnemyDestroy);
	ExistingEnemies.Emplace(SpawnedPawn);
	//运行BT（已废弃）
	/*
	if (!SpawnedPawn->Controller)
	{
		SpawnedPawn->SpawnDefaultController();
	}
	if (SpawnedPawn->IsPawnControlled())
	{
		AAIController* EnemyController = Cast<AAIController>(SpawnedPawn->Controller);
		EnemyController ->RunBehaviorTree(EnemyBT);
	}
	*/
}

//pawn销毁调用
void AAFPEnemySpawnerActor::OnEnemyDestroy(AActor* DestroyedActor)
{
	if(ExistingEnemies.Contains(DestroyedActor))
	{
		ExistingEnemies.Remove(DestroyedActor);
	}
	switch (SpawnProfile->SpawnMode) {
	case EEnemySpawnMode::AutoStreak://自动波次
	{
		if (ExistingEnemies.IsEmpty())
		{
			if (CurrentStreak >= SpawnProfile->Streaks.Num())
			{
				OnEnemyEliminated.Broadcast();
				return;
			}
			else if(StreakConverted.IsEmpty())
			{
				OnWaveSwapped.Broadcast(CurrentStreak);
				Spawn_AutoStart();
			}
		}
	}
		break;
	case EEnemySpawnMode::MaxCount://最大数量
	{
		if (ExistingEnemies.IsEmpty()&& StreakConverted.IsEmpty())
		{
			OnEnemyEliminated.Broadcast();
			return;
		}
		if (ExistingEnemies.Num()< SpawnProfile->MaxCount)
		{
			Spawn_MaxCount();
		}
	}
		break;
	case EEnemySpawnMode::LoopInTime://限时循环
	{
		if (ExistingEnemies.IsEmpty())
		{
			if (CurrentStreak >= SpawnProfile->Streaks.Num())
			{
				OnEnemyEliminated.Broadcast();
				return;
			}
			else if (StreakConverted.IsEmpty())
			{
				OnWaveSwapped.Broadcast(CurrentStreak);
				Spawn_LoopInTime();
			}
		}
	}
		break;
	default:
		break;
	}

}
//生成第d个维度的第i个sobol数
float AAFPEnemySpawnerActor::Sobol(uint32 d, uint32 i) {
	const uint32 Matrix[8 * 32] = {
2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216, 8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1,
2147483648, 3221225472, 2684354560, 4026531840, 2281701376, 3422552064, 2852126720, 4278190080, 2155872256, 3233808384, 2694840320, 4042260480, 2290614272, 3435921408, 2863267840, 4294901760, 2147516416, 3221274624, 2684395520, 4026593280, 2281736192, 3422604288, 2852170240, 4278255360, 2155905152, 3233857728, 2694881440, 4042322160, 2290649224, 3435973836, 2863311530, 4294967295,
2147483648, 3221225472, 1610612736, 2415919104, 3892314112, 1543503872, 2382364672, 3305111552, 1753219072, 2629828608, 3999268864, 1435500544, 2154299392, 3231449088, 1626210304, 2421489664, 3900735488, 1556135936, 2388680704, 3314585600, 1751705600, 2627492864, 4008611328, 1431684352, 2147543168, 3221249216, 1610649184, 2415969680, 3892340840, 1543543964, 2382425838, 3305133397,
2147483648, 3221225472, 536870912, 1342177280, 4160749568, 1946157056, 2717908992, 2466250752, 3632267264, 624951296, 1507852288, 3872391168, 2013790208, 3020685312, 2181169152, 3271884800, 546275328, 1363623936, 4226424832, 1977167872, 2693105664, 2437829632, 3689389568, 635137280, 1484783744, 3846176960, 2044723232, 3067084880, 2148008184, 3222012020, 537002146, 1342505107,
2147483648, 1073741824, 536870912, 2952790016, 4160749568, 3690987520, 2046820352, 2634022912, 1518338048, 801112064, 2707423232, 4038066176, 3666345984, 1875116032, 2170683392, 1085997056, 579305472, 3016343552, 4217741312, 3719483392, 2013407232, 2617981952, 1510979072, 755882752, 2726789248, 4090085440, 3680870432, 1840435376, 2147625208, 1074478300, 537900666, 2953698205,
2147483648, 1073741824, 1610612736, 805306368, 2818572288, 335544320, 2113929216, 3472883712, 2290089984, 3829399552, 3059744768, 1127219200, 3089629184, 4199809024, 3567124480, 1891565568, 394297344, 3988799488, 920674304, 4193267712, 2950604800, 3977188352, 3250028032, 129093376, 2231568512, 2963678272, 4281226848, 432124720, 803643432, 1633613396, 2672665246, 3170194367,
2147483648, 3221225472, 2684354560, 3489660928, 1476395008, 2483027968, 1040187392, 3808428032, 3196059648, 599785472, 505413632, 4077912064, 1182269440, 1736704000, 2017853440, 2221342720, 3329785856, 2810494976, 3628507136, 1416089600, 2658719744, 864310272, 3863387648, 3076993792, 553150080, 272922560, 4167467040, 1148698640, 1719673080, 2009075780, 2149644390, 3222291575,
2147483648, 1073741824, 2684354560, 1342177280, 2281701376, 1946157056, 436207616, 2566914048, 2625634304, 3208642560, 2720006144, 2098200576, 111673344, 2354315264, 3464626176, 4027383808, 2886631424, 3770826752, 1691164672, 3357462528, 1993345024, 3752330240, 873073152, 2870150400, 1700563072, 87021376, 1097028000, 1222351248, 1560027592, 2977959924, 23268898, 437609937
	};
	uint32 result = 0;
	uint32 offset = d * 32;
	for (uint32 j = 0; i; i >>= 1, j++)
		if (i & 1)
			result ^= Matrix[j + offset];
	return float(result) * (1.0f / float(0xFFFFFFFFU));
}
//生成二维sobol坐标
FVector2D AAFPEnemySpawnerActor::SobolVec2D(uint32 i)
{
	float u = Sobol(1, i ^ (i >> 1));
	float v = Sobol(2, i ^ (i >> 1));
	return FVector2D(u, v);
}
