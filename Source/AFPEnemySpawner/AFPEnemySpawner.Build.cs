// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AFPEnemySpawner : ModuleRules
{
	public AFPEnemySpawner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
			);
	}
}
