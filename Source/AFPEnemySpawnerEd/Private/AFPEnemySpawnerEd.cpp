// Skylake Game Studio

#include "AFPEnemySpawnerEd.h"
#include "AFPEnemySpawnerActor.h"
#include "AFPEnemySpawnerEditor.h"
#include "IPlacementModeModule.h"

#define LOCTEXT_NAMESPACE "FAFPEnemySpawnerEdModule"

void FAFPEnemySpawnerEdModule::StartupModule()
{
    //Add to placement panel
    const FPlacementCategoryInfo Info(
        INVTEXT("AFP Enemy Spawn"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "PlacementBrowser.Icons.Basic"),
        "AFPEnemySpawn",
        TEXT("AFPEnemySpawn"),
        0
    );

    IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
    PlacementModeModule.RegisterPlacementCategory(Info);

    PlacementModeModule.RegisterPlaceableItem(Info.UniqueHandle,
        MakeShared<FPlaceableItem>(nullptr, FAssetData(AAFPEnemySpawnerActor::StaticClass())));
    PlacementModeModule.RegisterPlaceableItem(Info.UniqueHandle,
        MakeShared<FPlaceableItem>(nullptr, FAssetData(AAFPEnemySpawnerEditor::StaticClass())));
}

void FAFPEnemySpawnerEdModule::ShutdownModule()
{
    if (IPlacementModeModule::IsAvailable())
    {
        IPlacementModeModule::Get().UnregisterPlacementCategory("AFPEnemySpawn");
    }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAFPEnemySpawnerEdModule, AFPEnemySpawnerEd)