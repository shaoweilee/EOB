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
			"GameplayTasks",
			// M6b：MassBattle 割草体系（引擎级插件，启用后直接引用其模块）
			"MassBattle", // 刷怪/伤害/事件 API（UMassBattleFuncLib）
			"MassAPI", // 实体句柄 FEntityHandle 等基础类型
			"FlowFieldCanvas", // 流场 AFlowField（群怪寻路）
			"MassEntity" // Mass 实体框架基础头文件
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