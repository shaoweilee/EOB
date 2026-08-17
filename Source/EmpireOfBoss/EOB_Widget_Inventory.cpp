#include "EOB_Widget_Inventory.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_Widget_InventorySlot.h"
#include "EOB_Widget_InventoryTab.h"
#include "EOB_Widget_HeldItemIcon.h"
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
	if (GridPanel_Items)
	{
		if (UCanvasPanelSlot* GridSlot = Cast<UCanvasPanelSlot>(GridPanel_Items->Slot))
		{
			if (!GridSlot->GetAutoSize())
			{
				const FAnchors Anchors = GridSlot->GetAnchors();
				const bool bTopLeftAnchors = Anchors.Minimum.Equals(FVector2D(0.f, 0.f))
					&& Anchors.Maximum.Equals(FVector2D(0.f, 0.f));

				if (bTopLeftAnchors)
				{
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

	// ── 手持物品图标：优先用蓝图子类 WBP_HeldItemIcon（在类默认值 HeldIconWidgetClass 里指定），
	//    留空则退化为纯 C++ 类（NativeConstruct 自建 SizeBox+Image） ──
	TSubclassOf<UEOB_Widget_HeldItemIcon> IconClass = HeldIconWidgetClass;
	if (!IconClass)
	{
		IconClass = UEOB_Widget_HeldItemIcon::StaticClass();
	}

	HeldIcon = CreateWidget<UEOB_Widget_HeldItemIcon>(GetOwningPlayer(), IconClass);
	if (HeldIcon)
	{
		HeldIcon->AddToViewport(1000); // 画在所有面板之上；默认 Collapsed，拿起时才显示
		UE_LOG(LogTemp, Log, TEXT("[手持] 图标控件创建成功（类：%s），已加入视口"), *IconClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[手持] 图标控件创建失败！CreateWidget 返回空。"));
	}

	if (!HeldIconWidgetClass)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "[手持] 未设置 HeldIconWidgetClass，正在用纯 C++ 兜底类。建议创建 WBP_HeldItemIcon（父类 EOB_Widget_HeldItemIcon）并在 WBP_Inventory 类默认值里指定。"
		       ));
	}

	RefreshUI();
}

void UEOB_Widget_Inventory::NativeDestruct()
{
	if (HeldIcon)
	{
		HeldIcon->RemoveFromParent();
		HeldIcon = nullptr;
	}
	Super::NativeDestruct();
}

void UEOB_Widget_Inventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FSlateApplication::IsInitialized()) return;

	const bool bLeftDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);
	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	// 按下后位移超过阈值：进入拖拽；手上还没东西就从起点格子拿起来
	if (PotentialDragSlot.IsValid() && bLeftDown && !bDragging
		&& FVector2D::Distance(CursorPos, PotentialDragPos) >= DragThresholdPixels)
	{
		bDragging = true;
		UE_LOG(LogTemp, Log, TEXT("[手持] 位移超过 %.0fpx，进入拖拽"), DragThresholdPixels);
		if (!IsHoldingItem())
		{
			TryPickUpFromSlot(PotentialDragSlot.Get());
		}
	}

	// 松开左键：拖拽的落点结算（原地点击不走这里，由格子的 OnClicked 走抓取逻辑）
	if (!bLeftDown)
	{
		if (bDragging)
		{
			bDragging = false;
			if (IsHoldingItem())
			{
				ResolveDropAtScreenPosition(CursorPos);
			}
		}
		PotentialDragSlot = nullptr;
	}

	// 手持时的"禁止投放"红框提示（覆盖所有面板的所有格子，含装备栏位）
	UpdateForbiddenHighlights(CursorPos);
}

void UEOB_Widget_Inventory::SetCurrentTab(int32 NewTab)
{
	if (!RefInventory) return;
	if (NewTab < 0 || NewTab >= UEOB_InventoryComponent::NumTabs) return;

	if (!RefInventory->IsTabActive(NewTab))
	{
		UE_LOG(LogTemp, Log, TEXT("[背包 UI] 第 %d 个栏位没有背包，不能切换过去。"), NewTab + 1);
		return;
	}

	CurrentTabIndex = NewTab;
	CloseAllTabDropdowns();
	RefreshUI();
}

void UEOB_Widget_Inventory::CloseAllTabDropdowns(int32 ExceptTabIndex)
{
	auto CloseInBox = [ExceptTabIndex](UVerticalBox* Box)
	{
		if (!Box) return;
		for (UWidget* Child : Box->GetAllChildren())
		{
			if (UEOB_Widget_InventoryTab* TabWidget = Cast<UEOB_Widget_InventoryTab>(Child))
			{
				if (TabWidget->GetTabIndex() != ExceptTabIndex)
				{
					TabWidget->CloseDropdown();
				}
			}
		}
	};

	CloseInBox(VerticalBox_TabsLeft);
	CloseInBox(VerticalBox_TabsRight);
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

void UEOB_Widget_Inventory::RefreshUI_Implementation()
{
	if (!GridPanel_Items || !SlotWidgetClass || !RefInventory) return;

	GridPanel_Items->ClearChildren();

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

	if (RefInventory->IsTabActive(CurrentTabIndex))
	{
		const int32 Capacity = RefInventory->GetTabCapacity(CurrentTabIndex);
		for (int32 i = 0; i < Capacity; ++i)
		{
			UEOB_Widget_InventorySlot* SlotWidget = CreateWidget<UEOB_Widget_InventorySlot>(this, SlotWidgetClass);
			if (!SlotWidget) continue;

			FEOBItemInstance Item;
			RefInventory->GetItemAt(CurrentTabIndex, i, Item);

			SlotWidget->SetSlotDesiredSize(InventorySlotSize);

			SlotWidget->InitInventorySlot(RefInventory, CurrentTabIndex, i, this);
			SlotWidget->UpdateSlot(Item);
			GridPanel_Items->AddChildToUniformGrid(SlotWidget, i / GridColumns, i % GridColumns);
		}
	}

	RefreshTabs();
}

// ===================== 抓取 / 拖拽 =====================

void UEOB_Widget_Inventory::NotifySlotLeftPressed(UEOB_Widget_InventorySlot* InSlot, const FVector2D& ScreenPos)
{
	PotentialDragSlot = InSlot;
	PotentialDragPos = ScreenPos;
	UE_LOG(LogTemp, Log, TEXT("[手持] 登记拖拽起点：模式=%d，页=%d，格=%d"),
	       static_cast<int32>(InSlot->GetMode()), InSlot->GetTabIndex(), InSlot->GetSlotInTab());
}

void UEOB_Widget_Inventory::OnSlotGrabClicked(UEOB_Widget_InventorySlot* InSlot)
{
	// 拖拽中松开会触发起点按钮的 OnClicked（UMG 按钮默认 DownAndUp，松手即触发），
	// 这次"点击"必须忽略——拖拽的落点由 Tick 里的 ResolveDropAtScreenPosition 结算。
	if (bDragging) return;

	UE_LOG(LogTemp, Log, TEXT("[手持] 原地点击：模式=%d，页=%d，格=%d，当前%s"),
	       static_cast<int32>(InSlot->GetMode()), InSlot->GetTabIndex(), InSlot->GetSlotInTab(),
	       IsHoldingItem() ? TEXT("手上有东西→放下") : TEXT("手上空→拿起"));

	if (IsHoldingItem())
	{
		// 方案一：抓取手势点到非法格子（红框）时保持手持，主人可换目标或右键取消
		PlaceHeldOnSlot(InSlot);
	}
	else
	{
		TryPickUpFromSlot(InSlot);
	}
}

void UEOB_Widget_Inventory::CancelHeldItem()
{
	ClearHeldItem(true);
}

void UEOB_Widget_Inventory::TryPickUpFromSlot(UEOB_Widget_InventorySlot* InSlot)
{
	if (!RefInventory || !InSlot || IsHoldingItem()) return;

	FEOBItemInstance Item;
	EEOBHeldSourceType SourceType = EEOBHeldSourceType::None;

	if (InSlot->GetMode() == EEOBSlotWidgetMode::Inventory)
	{
		if (RefInventory->GetItemAt(InSlot->GetTabIndex(), InSlot->GetSlotInTab(), Item) && Item.IsValid())
		{
			SourceType = EEOBHeldSourceType::InventorySlot;
			HeldTab = InSlot->GetTabIndex();
			HeldSlot = InSlot->GetSlotInTab();
		}
	}
	else if (InSlot->GetMode() == EEOBSlotWidgetMode::BagSlot)
	{
		if (RefInventory->GetTabBag(InSlot->GetTabIndex(), Item) && Item.IsValid())
		{
			SourceType = EEOBHeldSourceType::BagSlot;
			HeldTab = InSlot->GetTabIndex();
			HeldSlot = INDEX_NONE;
		}
	}

	if (SourceType == EEOBHeldSourceType::None || !Item.Definition)
	{
		UE_LOG(LogTemp, Log, TEXT("[手持] 拿起失败：这一格是空的（或没有 Definition）"));
		return;
	}

	// 注意：东西仍在原格（数据不动），只是源格显示为空 + 图标跟手。
	HeldSource = SourceType;
	HeldGhostSlot = InSlot;
	InSlot->UpdateSlot(FEOBItemInstance());
	BeginHeldIcon(Item.Definition->Icon);

	UE_LOG(LogTemp, Log, TEXT("[手持] 已拿起【%s】（来源=%s）"),
	       *Item.Definition->ItemName.ToString(),
	       SourceType == EEOBHeldSourceType::BagSlot ? TEXT("包裹栏位") : TEXT("背包格"));
}

bool UEOB_Widget_Inventory::PlaceHeldOnSlot(UEOB_Widget_InventorySlot* TargetSlot)
{
	if (!RefInventory || !TargetSlot || !IsHoldingItem()) return false;

	const EEOBSlotWidgetMode TargetMode = TargetSlot->GetMode();
	const int32 TargetTab = TargetSlot->GetTabIndex();
	const int32 TargetSlotInTab = TargetSlot->GetSlotInTab();

	// ── 来源：背包格 ──
	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		if (TargetMode == EEOBSlotWidgetMode::Inventory)
		{
			if (TargetTab == HeldTab && TargetSlotInTab == HeldSlot)
			{
				ClearHeldItem(true); // 点回源格 = 原路放回
				return true;
			}
			if (RefInventory->MoveOrSwapItem(HeldTab, HeldSlot, TargetTab, TargetSlotInTab))
			{
				ClearHeldItem(false);
				return true;
			}
			return false;
		}

		if (TargetMode == EEOBSlotWidgetMode::BagSlot)
		{
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid()
				&& Item.Definition && Item.Definition->Kind == EEOBItemKind::Bag)
			{
				if (RefInventory->EquipBagFromSlotToTab(HeldTab, HeldSlot, TargetTab))
				{
					ClearHeldItem(false);
					return true;
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[背包 UI] 包裹栏位只能放背包物品。"));
			}
			return false;
		}

		return false;
	}

	// ── 来源：包裹栏位 ──
	if (HeldSource == EEOBHeldSourceType::BagSlot)
	{
		if (TargetMode == EEOBSlotWidgetMode::BagSlot)
		{
			if (TargetTab == HeldTab)
			{
				ClearHeldItem(true); // 点回源栏位 = 原路放回
				return true;
			}
			if (RefInventory->SwapTabs(HeldTab, TargetTab))
			{
				ClearHeldItem(false);
				return true;
			}
			return false;
		}

		if (TargetMode == EEOBSlotWidgetMode::Inventory)
		{
			// 方案一：禁止把包放进它自己那一页——否则该页失去包裹变未激活，
			// 包会被藏进一页看不见摸不着的背包里（悬停时红框已提示）。
			if (TargetTab == HeldTab)
			{
				UE_LOG(LogTemp, Log, TEXT("[背包 UI] 不能把包裹放进它自己那一页。"));
				return false;
			}
			if (RefInventory->UnequipBagToSlot(HeldTab, TargetTab, TargetSlotInTab))
			{
				ClearHeldItem(false);
				return true;
			}
			return false;
		}
	}

	return false;
}

void UEOB_Widget_Inventory::ResolveDropAtScreenPosition(const FVector2D& ScreenPos)
{
	if (!IsHoldingItem()) return;

	if (UEOB_Widget_InventorySlot* TargetSlot = FindSlotAtScreenPosition(ScreenPos))
	{
		// 拖拽松手：落点合法就放下；落点是非法格子（红框）就取消手持、东西回原位
		if (!PlaceHeldOnSlot(TargetSlot))
		{
			ClearHeldItem(true);
		}
		return;
	}

	// 背包面板内没找到 → 看看是不是落在"其他面板的格子"上（比如装备栏位）：
	// 那种地方不接受投放（红框已提示），按取消处理——东西回原处，绝不误丢到地上。
	for (const TWeakObjectPtr<UEOB_Widget_InventorySlot>& SlotPtr : UEOB_Widget_InventorySlot::GetAllSlotWidgets())
	{
		UEOB_Widget_InventorySlot* AnySlot = SlotPtr.Get();
		if (!AnySlot) continue;
		const ESlateVisibility Vis = AnySlot->GetVisibility();
		if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) continue;
		if (AnySlot->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			ClearHeldItem(true);
			return;
		}
	}

	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		const bool bOutsidePanel = !GetCachedGeometry().IsUnderLocation(ScreenPos);
		if (bOutsidePanel && RefInventory)
		{
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid())
			{
				if (RefInventory->DropItemToWorld(HeldTab, HeldSlot))
				{
					ClearHeldItem(false);
					return;
				}
			}
		}
		ClearHeldItem(true);
		return;
	}

	ClearHeldItem(true);
}

UEOB_Widget_InventorySlot* UEOB_Widget_Inventory::FindSlotAtScreenPosition(const FVector2D& ScreenPos) const
{
	auto IsSlotUnderCursor = [&ScreenPos](UEOB_Widget_InventorySlot* SlotCandidate) -> bool
	{
		if (!SlotCandidate) return false;
		const ESlateVisibility Vis = SlotCandidate->GetVisibility();
		if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) return false;
		return SlotCandidate->GetCachedGeometry().IsUnderLocation(ScreenPos);
	};

	auto FindInTabs = [&IsSlotUnderCursor](UVerticalBox* Box) -> UEOB_Widget_InventorySlot*
	{
		if (!Box) return nullptr;
		for (UWidget* Child : Box->GetAllChildren())
		{
			if (UEOB_Widget_InventoryTab* TabWidget = Cast<UEOB_Widget_InventoryTab>(Child))
			{
				UEOB_Widget_InventorySlot* BagSlot = TabWidget->GetBagSlotWidget();
				if (IsSlotUnderCursor(BagSlot)) return BagSlot;
			}
		}
		return nullptr;
	};

	if (UEOB_Widget_InventorySlot* Found = FindInTabs(VerticalBox_TabsLeft)) return Found;
	if (UEOB_Widget_InventorySlot* Found = FindInTabs(VerticalBox_TabsRight)) return Found;

	if (GridPanel_Items)
	{
		for (UWidget* Child : GridPanel_Items->GetAllChildren())
		{
			if (UEOB_Widget_InventorySlot* SlotWidget = Cast<UEOB_Widget_InventorySlot>(Child))
			{
				if (IsSlotUnderCursor(SlotWidget)) return SlotWidget;
			}
		}
	}

	return nullptr;
}

// ===================== 禁止投放红框 =====================

void UEOB_Widget_Inventory::UpdateForbiddenHighlights(const FVector2D& ScreenPos)
{
	TArray<TWeakObjectPtr<UEOB_Widget_InventorySlot>>& AllSlots = UEOB_Widget_InventorySlot::GetAllSlotWidgets();

	// 顺手清掉已失效的弱引用（网格每次刷新都会重建格子）
	AllSlots.RemoveAllSwap([](const TWeakObjectPtr<UEOB_Widget_InventorySlot>& Ptr) { return !Ptr.IsValid(); });

	for (const TWeakObjectPtr<UEOB_Widget_InventorySlot>& SlotPtr : AllSlots)
	{
		UEOB_Widget_InventorySlot* SlotWidget = SlotPtr.Get();
		if (!SlotWidget) continue;

		bool bRed = false;
		if (IsHoldingItem())
		{
			const ESlateVisibility Vis = SlotWidget->GetVisibility();
			if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden
				&& SlotWidget->GetCachedGeometry().IsUnderLocation(ScreenPos))
			{
				bRed = !WouldAcceptDrop(SlotWidget);
			}
		}
		SlotWidget->SetForbiddenHighlight(bRed);
	}
}

bool UEOB_Widget_Inventory::WouldAcceptDrop(const UEOB_Widget_InventorySlot* TargetSlot) const
{
	if (!RefInventory || !TargetSlot || !IsHoldingItem()) return true;

	// 装备栏位：装备侧的抓取/投放还没做（下一阶段），目前一律不接受 → 悬停必红。
	// 以后做"禁止装备"（职业/等级/槽位不符）时，规则也加在这个分支里。
	if (TargetSlot->GetMode() == EEOBSlotWidgetMode::Equipment)
	{
		return false;
	}

	// 手上拿着的物品（懒转移模型：数据仍在原格，能直接查到）
	FEOBItemInstance HeldItem;
	bool bGotHeld = false;
	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		bGotHeld = RefInventory->GetItemAt(HeldTab, HeldSlot, HeldItem);
	}
	else if (HeldSource == EEOBHeldSourceType::BagSlot)
	{
		bGotHeld = RefInventory->GetTabBag(HeldTab, HeldItem);
	}
	if (!bGotHeld || !HeldItem.IsValid() || !HeldItem.Definition) return true;

	const bool bHeldIsBag = (HeldItem.Definition->Kind == EEOBItemKind::Bag);

	// 目标：包裹栏位
	if (TargetSlot->GetMode() == EEOBSlotWidgetMode::BagSlot)
	{
		if (HeldSource == EEOBHeldSourceType::BagSlot)
		{
			return true; // 包 ↔ 包裹栏位 = 互换页签位置，总是可以
		}
		return bHeldIsBag; // 背包格 → 包裹栏位：只有背包物品能放上去
	}

	// 目标：背包格，来源：包裹栏位（拿着的是已装备的包）
	if (HeldSource == EEOBHeldSourceType::BagSlot)
	{
		// 禁止放进它自己那一页（页会失活，包被藏起来）
		if (TargetSlot->GetTabIndex() == HeldTab) return false;

		// 包内非空不能卸下：此时任何背包格都不接受
		if (!IsHeldBagEmpty()) return false;

		// 目标格必须为空
		FEOBItemInstance TargetItem;
		if (RefInventory->GetItemAt(TargetSlot->GetTabIndex(), TargetSlot->GetSlotInTab(), TargetItem)
			&& TargetItem.IsValid())
		{
			return false;
		}
		return true;
	}

	// 背包格 → 背包格：移动/交换总是可以
	return true;
}

bool UEOB_Widget_Inventory::IsHeldBagEmpty() const
{
	if (!RefInventory || HeldSource != EEOBHeldSourceType::BagSlot) return true;

	const int32 Capacity = RefInventory->GetTabCapacity(HeldTab);
	for (int32 i = 0; i < Capacity; ++i)
	{
		FEOBItemInstance Item;
		if (RefInventory->GetItemAt(HeldTab, i, Item) && Item.IsValid())
		{
			return false;
		}
	}
	return true;
}

void UEOB_Widget_Inventory::BeginHeldIcon(UTexture2D* Icon)
{
	if (HeldIcon)
	{
		HeldIcon->ShowIcon(Icon, HeldIconSize);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[手持] 要显示图标时 HeldIcon 是空的！"));
	}
}

void UEOB_Widget_Inventory::ClearHeldItem(bool bRestoreSourceVisual)
{
	if (bRestoreSourceVisual && RefInventory && HeldGhostSlot.IsValid())
	{
		FEOBItemInstance Item;
		bool bGot = false;
		if (HeldSource == EEOBHeldSourceType::InventorySlot)
		{
			bGot = RefInventory->GetItemAt(HeldTab, HeldSlot, Item);
		}
		else if (HeldSource == EEOBHeldSourceType::BagSlot)
		{
			bGot = RefInventory->GetTabBag(HeldTab, Item);
		}
		HeldGhostSlot->UpdateSlot(bGot ? Item : FEOBItemInstance());
	}

	HeldSource = EEOBHeldSourceType::None;
	HeldTab = INDEX_NONE;
	HeldSlot = INDEX_NONE;
	HeldGhostSlot = nullptr;

	if (HeldIcon)
	{
		HeldIcon->HideIcon();
	}
}
