// Skylake Game Studio
#pragma once

#include "Engine/DataAsset.h"
#include "GameFramework/Pawn.h"
#include "AFPEnemySpawnProfile.generated.h"


USTRUCT(BlueprintType, Blueprintable, meta = (MakeEditWidget))
struct FEnemySubStreak
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn", meta = (MakeEditWidget))
		FTransform EnemyTransform = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		TSubclassOf<APawn> EnemyType = APawn::StaticClass();
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
	//UBehaviorTree* EnemyBT;将不使用外置BT，BT相关逻辑由敌人Pawn内部实现
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		int32 EnemyCount = 1;
};

UENUM(BlueprintType, Blueprintable, Category = "AFPSpawn")
enum class EEnemySpawnMode : uint8
{
	AutoStreak,
	MaxCount,
	LoopInTime

};

USTRUCT(BlueprintType, Blueprintable, Category = "AFPSpawn", meta = (MakeEditWidget))
struct FEnemyStreak
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		float StreakDelay = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		float DistributeTime = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		TArray<FEnemySubStreak> EnemyStreak;
};

UCLASS(BlueprintType, Category = "AFPSpawn")
class AFPENEMYSPAWNER_API UAFPEnemySpawnProfile : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		FText BattleName = FText();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn", meta = (MakeEditWidget))
		EEnemySpawnMode SpawnMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		int32 MaxCount = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		float Duration = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AFPSpawn")
		TArray<FEnemyStreak> Streaks;

};
