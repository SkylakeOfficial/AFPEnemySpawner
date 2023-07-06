// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AFPEnemySpawnerEd : ModuleRules
{
	public AFPEnemySpawnerEd(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AFPEnemySpawner",
                "PlacementMode"
			}
			);	
	}
}
