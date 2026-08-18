#include "EOB_HUDWidget.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_EquipmentPanel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EOB_Widget_CharacterPanel.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_LevelComponent.h"
#include "TimerManager.h"
#include "EOB_Widget_SkillTree.h"

namespace EOBHudZOrder
{
	/**
	 * 可开合面板（背包/装备/角色/技能树）的视口层级。
	 * 必须压过 HUD 本体（默认 0 层）里的常驻控件（动作条/小地图等）——
	 * 否则像"页签偏好下拉框"这种伸出面板边框的部分，会被动作条之类的 WBP 挡住、点不到。
	 * 手持图标（1000 层）仍在最上、点击层（-100 层）仍在最下，互不影响；
	 * 四个面板之间层级相同，相互遮挡关系与原来一致。
	 */
	static constexpr int32 ToggleablePanel = 10;
}

void UEOB_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动创建背包/装备面板，默认隐藏
	if (InventoryPanelClass && !InventoryPanel)
	{
		InventoryPanel = CreateWidget<UEOB_Widget_Inventory>(this, InventoryPanelClass);
		if (InventoryPanel)
		{
			InventoryPanel->AddToViewport(EOBHudZOrder::ToggleablePanel);
			InventoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EquipmentPanelClass && !EquipmentPanel)
	{
		EquipmentPanel = CreateWidget<UEOB_Widget_EquipmentPanel>(this, EquipmentPanelClass);
		if (EquipmentPanel)
		{
			EquipmentPanel->AddToViewport(EOBHudZOrder::ToggleablePanel);
			EquipmentPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// M3a：角色面板，默认隐藏（Synty UI 没有人物面板，自建）
	if (CharacterPanelClass && !CharacterPanel)
	{
		CharacterPanel = CreateWidget<UEOB_Widget_CharacterPanel>(this, CharacterPanelClass);
		if (CharacterPanel)
		{
			CharacterPanel->AddToViewport(EOBHudZOrder::ToggleablePanel);
			CharacterPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 进游戏先把经验条刷成 0 / 满级需求（等一帧，确保英雄和 LevelComponent 已就绪）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UEOB_HUDWidget::RefreshXPDisplay);
	}
	// M3b：技能树面板，默认隐藏
	if (SkillTreePanelClass && !SkillTreePanel)
	{
		SkillTreePanel = CreateWidget<UEOB_Widget_SkillTree>(this, SkillTreePanelClass);
		if (SkillTreePanel)
		{
			SkillTreePanel->AddToViewport(EOBHudZOrder::ToggleablePanel);
			SkillTreePanel->SetVisibility(ESlateVisibility::Collapsed);
		}
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

void UEOB_HUDWidget::ToggleSkillTreePanel()
{
	if (!SkillTreePanel) return;

	const bool bNowVisible = SkillTreePanel->IsVisible();
	// 🌟 穿透的教训：打开用 SelfHitTestInvisible
	const ESlateVisibility NewVis = bNowVisible
		                                ? ESlateVisibility::Collapsed
		                                : ESlateVisibility::SelfHitTestInvisible;
	SkillTreePanel->SetVisibility(NewVis);

	if (NewVis == ESlateVisibility::SelfHitTestInvisible)
	{
		SkillTreePanel->RefreshTree();
	}
}

void UEOB_HUDWidget::RefreshSkillTreePanel()
{
	if (SkillTreePanel && SkillTreePanel->IsVisible())
	{
		SkillTreePanel->RefreshTree();
	}
}
