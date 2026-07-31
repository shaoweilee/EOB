#include "EOB_HUDWidget.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_EquipmentPanel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EOB_Widget_CharacterPanel.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_LevelComponent.h"
#include "TimerManager.h"

void UEOB_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动创建背包/装备面板，默认隐藏
	if (InventoryPanelClass && !InventoryPanel)
	{
		InventoryPanel = CreateWidget<UEOB_Widget_Inventory>(this, InventoryPanelClass);
		if (InventoryPanel)
		{
			InventoryPanel->AddToViewport();
			InventoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EquipmentPanelClass && !EquipmentPanel)
	{
		EquipmentPanel = CreateWidget<UEOB_Widget_EquipmentPanel>(this, EquipmentPanelClass);
		if (EquipmentPanel)
		{
			EquipmentPanel->AddToViewport();
			EquipmentPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// M3a：角色面板，默认隐藏（Synty UI 没有人物面板，自建）
	if (CharacterPanelClass && !CharacterPanel)
	{
		CharacterPanel = CreateWidget<UEOB_Widget_CharacterPanel>(this, CharacterPanelClass);
		if (CharacterPanel)
		{
			CharacterPanel->AddToViewport();
			CharacterPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 进游戏先把经验条刷成 0 / 满级需求（等一帧，确保英雄和 LevelComponent 已就绪）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UEOB_HUDWidget::RefreshXPDisplay);
	}
}

void UEOB_HUDWidget::ToggleInventoryPanels()
{
	const bool bNowVisible = InventoryPanel && InventoryPanel->IsVisible();
	// 原来这里是 ESlateVisibility::Visible，改成 SelfHitTestInvisible
	const ESlateVisibility NewVis = bNowVisible
		                                ? ESlateVisibility::Collapsed
		                                : ESlateVisibility::SelfHitTestInvisible;

	if (InventoryPanel) InventoryPanel->SetVisibility(NewVis);
	if (EquipmentPanel) EquipmentPanel->SetVisibility(NewVis);
}

void UEOB_HUDWidget::ToggleCharacterPanel()
{
	if (!CharacterPanel) return;

	const bool bNowVisible = CharacterPanel->IsVisible();
	// 🌟 穿透的教训：打开用 SelfHitTestInvisible，不用 Visible
	const ESlateVisibility NewVis = bNowVisible
		                                ? ESlateVisibility::Collapsed
		                                : ESlateVisibility::SelfHitTestInvisible;
	CharacterPanel->SetVisibility(NewVis);

	// 打开瞬间先刷新一遍数据
	if (NewVis == ESlateVisibility::SelfHitTestInvisible)
	{
		CharacterPanel->RefreshFromHero();
	}
}

void UEOB_HUDWidget::RefreshXPDisplay()
{
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		if (Hero->LevelComponent)
		{
			// 🌟 驱动你在 Synty HUD 蓝图里已经接好的 BP_UpdateXP 事件
			BP_UpdateXP(FMath::RoundToInt(Hero->LevelComponent->CurrentXP),
			            FMath::RoundToInt(Hero->LevelComponent->GetXPToNextLevel()));
		}
	}
}

void UEOB_HUDWidget::RefreshCharacterPanel()
{
	if (CharacterPanel && CharacterPanel->IsVisible())
	{
		CharacterPanel->RefreshFromHero();
	}
}
