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
	// AEOPBaseCharacter* MyHero = Cast<AEOPBaseCharacter>(PC->GetPawn());
	// 兼容你PC里已经缓存的MyHero指针
	// if (!MyHero && PC->MyHero)
	// {
	// 	MyHero = Cast<AEOPBaseCharacter>(PC->MyHero);
	// }

	// if (MyHero)
	// {
	// 传送式设置位置，避免物理碰撞干扰
	PC->MyHero->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr,
	                                        ETeleportType::TeleportPhysics);

	// ======= 步骤2：显式触发GAS属性初始化 =======
	// 注意：这里会把EOPBaseCharacter里的自动初始化去掉，避免重复应用
	PC->MyHero->InitializeDefaultAttributes();

	// 把ASC指针同步给PC（兼容你PC里的UABC成员，避免PC里二次获取）
	// PC->UABC = MyHero->GetAbilitySystemComponent();
	// PC->MyHero = Cast<AEmpireOfBossCharacter>(MyHero);
	// PC->MoveComp = MyHero->GetCharacterMovement();
	// }

	// ======= 步骤3：统一构建UI + 首帧强刷血蓝条 =======
	AEmpireOfBossHUD* EOBHUD = Cast<AEmpireOfBossHUD>(PC->GetHUD());
	if (EOBHUD)
	{
		// 显式命令HUD创建UI（HUD里会把自动创建去掉）
		EOBHUD->InitializeHUDWidgets();

		UEOB_HUDWidget* HUDWidget = EOBHUD->GetHUDWidgetInstance();
		// 把UI实例同步给PC，方便PC里的属性变化回调直接调用
		PC->EOBHUDWidget = HUDWidget;

		if (HUDWidget && PC->MyHero && PC->MyHero->GetEOBAttributeSet())
		{
			const UEOB_AttributeSet* AttribSet = PC->MyHero->GetEOBAttributeSet();

			// 属性已100%初始化完成，计算首帧精确比例，杜绝0值闪烁
			float HPPercent = (AttribSet->GetMaxHealth() > 0.f)
				                  ? (AttribSet->GetHealth() / AttribSet->GetMaxHealth())
				                  : 1.f;
			float MPPercent = (AttribSet->GetMaxMana() > 0.f)
				                  ? (AttribSet->GetMana() / AttribSet->GetMaxMana())
				                  : 1.f;

			// 首帧强刷UI视觉
			HUDWidget->VM_UpdateHPVisual(HPPercent);
			HUDWidget->VM_UpdateMPVisual(MPPercent);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("🌟 [EOB Subsystem] 玩家全套状态及UI初始化圆满编排完成！"));
}
