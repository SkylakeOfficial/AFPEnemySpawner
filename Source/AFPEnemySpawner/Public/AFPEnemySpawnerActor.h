// Skylake Game Studio

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AFPEnemySpawnProfile.h"
#include "Components/BillboardComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "AFPEnemySpawnerActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAFPEnemyEliminatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAFPSpawnCompleteSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAFPSwapWaveSignature, int32, WaveIndex);

DECLARE_LOG_CATEGORY_EXTERN(AFPEnemySpawner,Log,All);

struct FSpawnConfig_Converted
{
	TSubclassOf<APawn> EnemyType;
	FTransform EnemyTransform = FTransform();
	//UBehaviorTree* EnemyBT;
};



UCLASS()
class AFPENEMYSPAWNER_API AAFPEnemySpawnerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AAFPEnemySpawnerActor();

	UPROPERTY(VisibleAnywhere, Category = "AFPSpawn")
	UBillboardComponent* SceneRoot;

	UPROPERTY(EditAnywhere, Category = "AFPSpawn")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, Category = "AFPSpawn" )
	TArray< UAFPEnemySpawnProfile*> SpawnProfileList;

	UFUNCTION(BlueprintCallable, Category = "AFPSpawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AFPSpawn")
	FText GetBattleName();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AFPSpawn")
	TArray<AActor*> GetSurvivingEnemies();	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AFPSpawn")
	float GetRemainingTime();

	UPROPERTY(BlueprintAssignable, Category = "AFPSpawn")
	FAFPEnemyEliminatedSignature OnEnemyEliminated;
	UPROPERTY(BlueprintAssignable, Category = "AFPSpawn")
	FAFPSpawnCompleteSignature OnSpawnCompleted;
	UPROPERTY(BlueprintAssignable, Category = "AFPSpawn")
	FAFPSwapWaveSignature OnWaveSwapped;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	TArray<FSpawnConfig_Converted> ConvertSpawnConfigs(TArray<FEnemySubStreak> InConfig);
	FVector2D SobolVec2D(uint32 i);
	float Sobol(uint32 d, uint32 i);


	void Spawn_AutoStart();
	UFUNCTION()
	void Spawn_AutoStart_Inner();
	void Spawn_MaxCount();
	UFUNCTION()
	void Spawn_MaxCount_Inner();
	void Spawn_LoopInTime();
	UFUNCTION()
	void Spawn_LoopInTime_Inner();
	UFUNCTION()
	void Spawn_LoopInTime_TimeUp();
	void Spawn(FSpawnConfig_Converted SpawnConfig);

	UFUNCTION()
	void OnEnemyDestroy(AActor* DestroyedActor);

	UAFPEnemySpawnProfile* SpawnProfile = nullptr;
	bool SpawnStarted = false;
	int32 CurrentStreak = 0;
	TArray<AActor*> ExistingEnemies;
	FTimerHandle EnemySpawnTimer;
	FTimerHandle TimeLimitTimer;
	TArray< FSpawnConfig_Converted> StreakConverted;
};
