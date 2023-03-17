// Skylake Game Studio

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AFPEnemySpawnProfile.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"
#include "AFPEnemySpawnerEditor.generated.h"

UCLASS()
class AFPENEMYSPAWNER_API AAFPEnemySpawnerEditor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAFPEnemySpawnerEditor();
	UPROPERTY(VisibleAnywhere, Category = "AFPSpawn")
	UBillboardComponent* SceneRoot;

	UPROPERTY(EditAnywhere, Category = "AFPSpawn")
	FText BattleName = FText();
	UPROPERTY(EditAnywhere, Category = "AFPSpawn")
	EEnemySpawnMode SpawnMode = EEnemySpawnMode::AutoStreak;
	UPROPERTY(EditAnywhere, Category = "AFPSpawn")
	int32 MaxCount = 10;
	UPROPERTY(EditAnywhere, Category = "AFPSpawn")
	float Duration = 0;
	UPROPERTY(EditAnywhere, Category = "AFPSpawn", meta = (DisplayName = "ProfileEditing"))
	UAFPEnemySpawnProfile* StoredProfile;
	UPROPERTY(EditAnywhere, Category = "AFPSpawn", meta = (DisplayName = "CurrentStreakSetting"), meta = (MakeEditWidget))
	FEnemyStreak EditingStreak;


	UFUNCTION(CallInEditor, Category = "AFPSpawn")
	void EditPrevious();
	UFUNCTION(CallInEditor, Category = "AFPSpawn")
	void EditNext();
	UFUNCTION(CallInEditor, Category = "AFPSpawn")
	void Apply();
	UFUNCTION(CallInEditor, Category = "AFPSpawn")
	void ToggleDisplayAll();


	virtual void OnConstruction(const FTransform& Transform) override;
protected:
	virtual void BeginPlay() override;
	int32 EditingStreakIndex = 0;
	UAFPEnemySpawnProfile* LastStoredProfile;
	UMaterial* TextMaterial;
	TArray<FColor> DebugColors;
	bool bDisplayAll = false;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
