// Skylake Game Studio


#include "AFPEnemySpawnerActor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "AIController.h"


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
	case EEnemySpawnMode::AutoStreak://�Զ�����ģʽ
		Spawn_AutoStart();
		break;
	case EEnemySpawnMode::MaxCount://��������ģʽ
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
	case EEnemySpawnMode::LoopInTime://����ʱ��ģʽ
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
		Config.EnemyBT = InConfig[i].EnemyBT;
		Config.EnemyType = InConfig[i].EnemyType;
		FVector Scale = InConfig[i].EnemyTransform.GetScale3D().operator*(FVector(1.0,1.0,0.0));
		FVector Loc = InConfig[i].EnemyTransform.GetLocation();
		FRotator Rot = FRotator(InConfig[i].EnemyTransform.GetRotation());
		for (int j = 0; j < InConfig[i].EnemyCount; ++j) {
			FVector LocOffset = InConfig[i].EnemyCount == 1 ? FVector(0, 0, 0) : UKismetMathLibrary::MakeVector(UKismetMathLibrary::RandomFloatInRange(-1.0F,1.0F)*40, UKismetMathLibrary::RandomFloatInRange(-1.0F, 1.0F) * 40,0)*(Scale);
			Config.EnemyTransform = UKismetMathLibrary::MakeTransform(Loc.operator+(LocOffset), Rot);
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
	UBehaviorTree* EnemyBT = SpawnConfig.EnemyBT;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	//生成
	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(SpawnConfig.EnemyType, SpawnLoc, SpawnRot, SpawnParams);
	//吸附到地面
	FVector ActorBounds;
	SpawnedPawn->GetActorBounds(true, SpawnLoc, ActorBounds, false);
	SpawnedPawn->SetActorLocation(Result.Location.operator+(FVector(0, 0, ActorBounds.Z)),true, nullptr, ETeleportType::TeleportPhysics);
	//FString dd = SpawnLoc.ToString();//Debug
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, FString::Printf(TEXT("%s"), *dd));
	//绑定到actor摧毁的委托
	SpawnedPawn->OnDestroyed.AddDynamic(this, &AAFPEnemySpawnerActor::OnEnemyDestroy);
	ExistingEnemies.Emplace(SpawnedPawn);
	//运行BT
	if (!SpawnedPawn->Controller)
	{
		SpawnedPawn->SpawnDefaultController();
	}
	if (SpawnedPawn->IsPawnControlled())
	{
		AAIController* EnemyController = Cast<AAIController>(SpawnedPawn->Controller);
		EnemyController ->RunBehaviorTree(EnemyBT);
	}
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


