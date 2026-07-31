#include "EOB_GameInitSubsystem.h"
#include "EmpireOfBossPlayerController.h"
#include "EOPBaseCharacter.h"
#include "EmpireOfBossHUD.h"
#include "EOB_HUDWidget.h"
#include "EOB_AttributeSet.h"

void UEOB_GameInitSubsystem::OrchestratePlayerInitialization(AEmpireOfBossPlayerController* PC, FVector SpawnLocation,
                                                             FRotator SpawnRotation)
{
	if (!PC) return;

	// ======= 步骤1：角色空间传送，保证位置先到位 =======

	// 传送式设置位置，避免物理碰撞干扰
	PC->MyHero->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr,
	                                        ETeleportType::TeleportPhysics);

	// ======= 步骤2：显式触发GAS属性初始化 =======
	// 注意：这里会把EOPBaseCharacter里的自动初始化去掉，避免重复应用
	PC->MyHero->InitializeDefaultAttributes();

	// ======= 步骤3：统一构建UI + 首帧强刷血蓝条 =======
	AEmpireOfBossHUD* EOBHUD = Cast<AEmpireOfBossHUD>(PC->GetHUD());
	UE_LOG(LogTemp, Warning, TEXT("[Init排查] GetHUD拿到的是: %s，Cast结果: %s"),
	       *GetNameSafe(PC->GetHUD()), EOBHUD ? TEXT("成功") : TEXT("失败"));

	if (EOBHUD)
	{
		// 显式命令HUD创建UI（HUD里会把自动创建去掉）
		EOBHUD->InitializeHUDWidgets();

		UEOB_HUDWidget* HUDWidget = EOBHUD->GetHUDWidgetInstance();
		PC->EOBHUDWidget = HUDWidget;

		UE_LOG(LogTemp, Warning, TEXT("[Init排查] HUDWidget: %s，MyHero: %s，AttributeSet: %s"),
		       *GetNameSafe(HUDWidget),
		       *GetNameSafe(PC->MyHero),
		       *GetNameSafe(PC->MyHero ? PC->MyHero->AttributeSet : nullptr));

		if (HUDWidget && PC->MyHero && PC->MyHero->AttributeSet)
		{
			const UEOB_AttributeSet* AttribSet = PC->MyHero->AttributeSet;

			float HPPercent = (AttribSet->GetMaxHealth() > 0.f)
				                  ? (AttribSet->GetHealth() / AttribSet->GetMaxHealth())
				                  : 1.f;
			float MPPercent = (AttribSet->GetMaxMana() > 0.f)
				                  ? (AttribSet->GetMana() / AttribSet->GetMaxMana())
				                  : 1.f;

			UE_LOG(LogTemp, Warning, TEXT("[Init排查] 已走到首帧强刷！HP=%.2f MP=%.2f"), HPPercent, MPPercent);

			HUDWidget->VM_UpdateHPVisual(HPPercent);
			HUDWidget->VM_UpdateMPVisual(MPPercent);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("🌟 [EOB Subsystem] 玩家全套状态及UI初始化圆满编排完成！"));
}
