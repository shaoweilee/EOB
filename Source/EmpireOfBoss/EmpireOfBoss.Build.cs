// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EmpireOfBoss : ModuleRules
{
	public EmpireOfBoss(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[]
		{
			"EmpireOfBoss",
			"EmpireOfBoss/Variant_Strategy",
			"EmpireOfBoss/Variant_Strategy/UI",
			"EmpireOfBoss/Variant_TwinStick",
			"EmpireOfBoss/Variant_TwinStick/AI",
			"EmpireOfBoss/Variant_TwinStick/Gameplay",
			"EmpireOfBoss/Variant_TwinStick/UI"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}