#include "EOB_Widget_Inventory.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_InventorySlot.h"
#include "EOB_Widget_InventoryTab.h"
#include "EmpireOfBossCharacter.h"

void UEOB_Widget_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动拿到主角的背包组件并订阅变化事件
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		RefInventory = Hero->InventoryComponent;
		if (RefInventory)
		{
			RefInventory->OnInventoryChanged.AddDynamic(this, &UEOB_Widget_Inventory::RefreshUI);
			RefInventory->OnEquipmentChanged.AddDynamic(this, &UEOB_Widget_Inventory::RefreshUI);
		}
	}

	// ── 格子网格：按内容自适应高度，杜绝“行数少时每行被拉高” ──
	// UniformGridPanel 会把自己拿到的全部空间平均分给【现有】的行，
	// 所以 20 格（3 行）时每行会被分到 440/3 ≈ 146 高。
	// 勾了“大小到内容”后，网格高度 = 行数 × 格子期望高度。
	if (GridPanel_Items)
	{
		if (UCanvasPanelSlot* GridSlot = Cast<UCanvasPanelSlot>(GridPanel_Items->Slot))
		{
			// 设计器里已经手动勾好就不用管（锚点/位置也在设计器里摆好了）
			if (!GridSlot->GetAutoSize())
			{
				const FAnchors Anchors = GridSlot->GetAnchors();
				const bool bTopLeftAnchors = Anchors.Minimum.Equals(FVector2D(0.f, 0.f))
					&& Anchors.Maximum.Equals(FVector2D(0.f, 0.f));

				if (bTopLeftAnchors)
				{
					// 保险措施：左上角锚点且没勾时，自动换算位置并开启
					const FVector2D TopLeft = GridSlot->GetPosition() - GridSlot->GetAlignment() * GridSlot->GetSize();
					GridSlot->SetAlignment(FVector2D(0.f, 0.f));
					GridSlot->SetPosition(TopLeft);
					GridSlot->SetAutoSize(true);
				}
				else
				{
					UE_LOG(LogTemp, Warning,
					       TEXT("[背包 UI] GridPanel_Items 没勾“大小到内容”，不满 32 格时格子会被拉伸，请在 WBP_Inventory 里勾选它。"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[背包 UI] GridPanel_Items 的父槽不是画布槽，请在 WBP_Inventory 里把它放进画布面板并勾选“大小到内容”。"));
		}
	}

	// ── 创建 12 个页签：左侧 6 个（第 1~6 页），右侧 6 个（第 7~12 页） ──
	const TSubclassOf<UEOB_Widget_InventoryTab> RightClass = TabWidgetClassRight
		                                                         ? TabWidgetClassRight
		                                                         : TabWidgetClassLeft;

	if (TabWidgetClassLeft && VerticalBox_TabsLeft && VerticalBox_TabsRight)
	{
		for (int32 Tab = 0; Tab < UEOB_InventoryComponent::NumTabs; ++Tab)
		{
			const bool bLeftSide = (Tab < UEOB_InventoryComponent::NumTabs / 2);
			const TSubclassOf<UEOB_Widget_InventoryTab> UseClass = bLeftSide ? TabWidgetClassLeft : RightClass;

			UEOB_Widget_InventoryTab* TabWidget = CreateWidget<UEOB_Widget_InventoryTab>(this, UseClass);
			if (!TabWidget) continue;

			TabWidget->InitTab(RefInventory, this, Tab);
			if (bLeftSide)
			{
				VerticalBox_TabsLeft->AddChild(TabWidget);
			}
			else
			{
				VerticalBox_TabsRight->AddChild(TabWidget);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "[背包 UI] 请在 WBP_Inventory 的类默认值里设置 TabWidgetClassLeft / TabWidgetClassRight，并摆好 VerticalBox_TabsLeft / VerticalBox_TabsRight 两个垂直框。"
		       ));
	}

	RefreshUI();
}

void UEOB_Widget_Inventory::SetCurrentTab(int32 NewTab)
{
	if (!RefInventory) return;
	if (NewTab < 0 || NewTab >= UEOB_InventoryComponent::NumTabs) return;

	// 没装背包的栏位不能被激活
	if (!RefInventory->IsTabActive(NewTab))
	{
		UE_LOG(LogTemp, Log, TEXT("[背包 UI] 第 %d 个栏位没有背包，不能切换过去。"), NewTab + 1);
		return;
	}

	CurrentTabIndex = NewTab;
	RefreshUI();
}

void UEOB_Widget_Inventory::RefreshTabs()
{
	auto RefreshBoxChildren = [](UVerticalBox* Box)
	{
		if (!Box) return;
		for (UWidget* Child : Box->GetAllChildren())
		{
			if (UEOB_Widget_InventoryTab* TabWidget = Cast<UEOB_Widget_InventoryTab>(Child))
			{
				TabWidget->RefreshTab();
			}
		}
	};

	RefreshBoxChildren(VerticalBox_TabsLeft);
	RefreshBoxChildren(VerticalBox_TabsRight);
}

void UEOB_Widget_Inventory::RefreshUI()
{
	if (!GridPanel_Items || !SlotWidgetClass || !RefInventory) return;

	GridPanel_Items->ClearChildren();

	// 当前页未激活（背包被卸了）：尝试自动跳回第一个激活页
	if (!RefInventory->IsTabActive(CurrentTabIndex))
	{
		for (int32 Tab = 0; Tab < UEOB_InventoryComponent::NumTabs; ++Tab)
		{
			if (RefInventory->IsTabActive(Tab))
			{
				CurrentTabIndex = Tab;
				break;
			}
		}
	}

	// 画当前页的格子：格子数 = 该页背包容量
	if (RefInventory->IsTabActive(CurrentTabIndex))
	{
		const int32 Capacity = RefInventory->GetTabCapacity(CurrentTabIndex);
		for (int32 i = 0; i < Capacity; ++i)
		{
			UEOB_Widget_InventorySlot* SlotWidget = CreateWidget<UEOB_Widget_InventorySlot>(this, SlotWidgetClass);
			if (!SlotWidget) continue;

			FEOBItemInstance Item;
			RefInventory->GetItemAt(CurrentTabIndex, i, Item);

			// 物品栏统一把格子实例定成 110×110（皮肤本身不定死，装备面板不受影响）
			SlotWidget->SetSlotDesiredSize(InventorySlotSize);

			SlotWidget->InitInventorySlot(RefInventory, CurrentTabIndex, i);
			SlotWidget->UpdateSlot(Item);
			GridPanel_Items->AddChildToUniformGrid(SlotWidget, i / GridColumns, i % GridColumns);
		}
	}

	// 页签状态跟着刷新（激活/禁用、当前页高亮、偏好名）
	RefreshTabs();
}
