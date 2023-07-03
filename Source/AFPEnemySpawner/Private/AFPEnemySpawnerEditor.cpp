// Skylake Game Studio


#include "AFPEnemySpawnerEditor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"

// Sets default values
AAFPEnemySpawnerEditor::AAFPEnemySpawnerEditor()
{
	PrimaryActorTick.bCanEverTick = true;
	LastStoredProfile = StoredProfile;
	SceneRoot = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root Comp"));//SceneRoot
	SetRootComponent(SceneRoot);
	SetActorHiddenInGame(true);
	static ConstructorHelpers::FObjectFinder<UMaterial> TextMat(TEXT("/AFPEnemySpawner/AFPEnemySpawnerTextMat"));
	if (TextMat.Succeeded())
	{
		TextMaterial = TextMat.Object;
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> BillboardTex(TEXT("/AFPEnemySpawner/Tx_Billboard"));
	if (BillboardTex.Succeeded())
	{
		SceneRoot->Sprite = BillboardTex.Object;
	}
	DebugColors = { FColor::FromHex("8A24FF"),FColor::FromHex("2323FA"),FColor::FromHex("308DFF"),FColor::FromHex("24E9FF"),FColor::FromHex("24FFAC")};
}

void AAFPEnemySpawnerEditor::EditNext()
{
#if WITH_EDITOR
	if (StoredProfile->Streaks.Num() <= EditingStreakIndex)
	{
		StoredProfile->Streaks.Emplace(EditingStreak);
	}
	else
	{
		StoredProfile->Streaks[EditingStreakIndex] = EditingStreak;
	}

	EditingStreakIndex += 1;

	if (StoredProfile->Streaks.Num() > EditingStreakIndex)
	{
		EditingStreak = StoredProfile->Streaks[EditingStreakIndex];
	}
	else
	{
		EditingStreak = FEnemyStreak();
	}

	RerunConstructionScripts();
#endif
}

void AAFPEnemySpawnerEditor::EditPrevious()
{
#if WITH_EDITOR
	if (StoredProfile->Streaks.Num() <= EditingStreakIndex)
	{
		StoredProfile->Streaks.Emplace(EditingStreak);
	}
	else
	{
		StoredProfile->Streaks[EditingStreakIndex] = EditingStreak;
	}

	EditingStreakIndex = UKismetMathLibrary::Clamp(EditingStreakIndex - 1, 0, 114514);

	if (StoredProfile->Streaks.Num() > EditingStreakIndex)
	{
		EditingStreak = StoredProfile->Streaks[EditingStreakIndex];
	}
	else
	{
		EditingStreak = FEnemyStreak();
	}

	RerunConstructionScripts();
#endif
}

void AAFPEnemySpawnerEditor::Apply()
{
#if WITH_EDITOR
	if (!StoredProfile)
	{
		return;
	} 
	else
	{
		StoredProfile->BattleName = BattleName;
		StoredProfile->SpawnMode = SpawnMode;
		StoredProfile->Duration = Duration;
		StoredProfile->MaxCount = MaxCount;
		if (StoredProfile->Streaks.Num() > EditingStreakIndex)
		{
			StoredProfile->Streaks[EditingStreakIndex] = EditingStreak;
		}
		else
		{
			StoredProfile->Streaks.Emplace(EditingStreak);
		}
	}
	RerunConstructionScripts();
#endif
}

void AAFPEnemySpawnerEditor::ToggleDisplayAll()
{
#if WITH_EDITOR
	if (bDisplayAll)
	{
		bDisplayAll = false;
	} 
	else
	{
		bDisplayAll = true;
	}
	RerunConstructionScripts();
#endif
}

void AAFPEnemySpawnerEditor::OnConstruction(const FTransform& Transform)
{
	TArray<FEnemyStreak> Streaks;
	if (!StoredProfile) 
	{
		EditingStreakIndex = 0;
		EditingStreak = FEnemyStreak();
		MaxCount = 10;
		Duration = 0;
		BattleName = FText();
		SpawnMode = EEnemySpawnMode::AutoStreak;
	}
	else if (LastStoredProfile != StoredProfile)
	{
		LastStoredProfile = StoredProfile;
		if (StoredProfile->Streaks.IsEmpty())
		{
			EditingStreak = FEnemyStreak();
		}
		else
		{
			EditingStreak = StoredProfile->Streaks[0];
		}
		EditingStreakIndex = 0;
		MaxCount = StoredProfile->MaxCount;
		Duration = StoredProfile->Duration;
		BattleName = StoredProfile->BattleName;
		SpawnMode = StoredProfile->SpawnMode;
		Streaks = StoredProfile->Streaks;
	}
	else
	{
		Streaks = StoredProfile->Streaks;
	}
	//���Ƶ�����Ϣ
	UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
	
		TSubclassOf<UActorComponent> TextCompClass = UTextRenderComponent::StaticClass();
		TArray< UActorComponent*> TextsGen = GetComponentsByClass(TextCompClass);

		for (int i = 0; i < TextsGen.Num(); i++)
		{
			if (TextsGen[i])
			{
				TextsGen[i]->UnregisterComponent();
				TextsGen[i]->DestroyComponent();
			}
		}
	if (Streaks.Num()>EditingStreakIndex)
	{
		for (int a = 0; a< Streaks.Num();a++)
		{
			if (bDisplayAll||a==EditingStreakIndex)
			{
				const TArray<FEnemySubStreak> SubStreaks = Streaks[a].EnemyStreak;
				if (!SubStreaks.IsEmpty())
				{
					for (int i = 0; i != SubStreaks.Num(); i++)
					{
						FVector Loc = SubStreaks[i].EnemyTransform.GetLocation().operator+(GetActorLocation()).operator+(FVector(0, 0, 30));
						FVector Scale = SubStreaks[i].EnemyTransform.GetScale3D().operator*(FVector(40, 40, 0)).operator+(FVector(0, 0, 30));
						FQuat Rot = SubStreaks[i].EnemyTransform.GetRotation();
						DrawDebugBox(GetWorld(), Loc, Scale, Rot, DebugColors[a % DebugColors.Num()], true);
						FString ClassName = SubStreaks[i].EnemyType->GetName();
						FString TextToDisplay = FString::Printf(TEXT("Streak %d\n%s x %d"), a, *ClassName, SubStreaks[i].EnemyCount);
						UTextRenderComponent* Text =NewObject<UTextRenderComponent>(this);
						AddInstanceComponent(Text);
						Text->RegisterComponent();
						Text->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
						if (TextMaterial)
						{
							Text->SetMaterial(0, TextMaterial);
						}
						Text->SetText(FText::FromString(TextToDisplay));
						Text->SetTextRenderColor(a == EditingStreakIndex ? FColor::Red : FColor::White);
						Text->SetWorldTransform(FTransform(Rot, Loc, FVector(1, 1, 1)));
						Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
						Text->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
					}
				}
			}
		}
	}
}

void AAFPEnemySpawnerEditor::BeginPlay()
{
	Super::BeginPlay();

	if (!WITH_EDITOR) K2_DestroyActor();
}

void AAFPEnemySpawnerEditor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

