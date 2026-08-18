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
#include "EOB_Widget_HoldClickCatcher.h"
#include "EmpireOfBossCharacter.h"

TWeakObjectPtr<UEOB_Widget_Inventory> UEOB_Widget_Inventory::Instance = nullptr;

TArray<TWeakObjectPtr<UWidget>>& UEOB_Widget_Inventory::GetAllPanelBoundsWidgets()
{
	/** 所有"代表面板范围"的控件（各面板的底图/外框，构造时自行登记） */
	static TArray<TWeakObjectPtr<UWidget>> GAllPanelBoundsWidgets;
	return GAllPanelBoundsWidgets;
}

void UEOB_Widget_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	// 登记全局唯一实例（装备面板的格子拿面板引用就靠它）
	Instance = this;

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

	// ── 全屏透明点击层：手持期间才显示，垫在所有面板之下、游戏画面之上 ──
	//    点面板外空地 = 丢弃/取消（顺便挡住对角色的移动指令）；右键 = 取消手持
	ClickCatcher = CreateWidget<UEOB_Widget_HoldClickCatcher>(GetOwningPlayer());
	if (ClickCatcher)
	{
		ClickCatcher->InitCatcher(this);
		ClickCatcher->AddToViewport(-100); // 压在所有面板之下
		ClickCatcher->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[手持] 点击捕捉层创建失败！手持时点面板外将无法丢弃。"));
	}

	RefreshUI();
}

void UEOB_Widget_Inventory::NativeDestruct()
{
	// 面板被销毁时手里还拿着装备：先穿回原始槽位，绝不吞装备
	if (HeldSource == EEOBHeldSourceType::EquipmentSlot && HeldEquipmentItem.IsValid() && RefInventory)
	{
		FEOBItemInstance Replaced;
		if (RefInventory->EquipInstanceToSlot(HeldEquipmentItem, HeldEquipSlot, Replaced) && Replaced.IsValid())
		{
			RefInventory->AddItem(Replaced); // 原位被占，占用者兜底入包
		}
	}

	if (Instance.Get() == this)
	{
		Instance = nullptr;
	}

	if (HeldIcon)
	{
		HeldIcon->RemoveFromParent();
		HeldIcon = nullptr;
	}
	if (ClickCatcher)
	{
		ClickCatcher->RemoveFromParent();
		ClickCatcher = nullptr;
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

	// 松开左键：结算本次按下。
	// 注意：格子的左键按下已在预览阶段被吞掉（Button 不捕获鼠标、也不会触发 OnClicked），
	// 所以"原地点击 = 抓取手势的拿起/放下"由这里补发。
	if (!bLeftDown)
	{
		if (PotentialDragSlot.IsValid())
		{
			if (bDragging)
			{
				// 拖拽松手：对落点结算
				bDragging = false;
				if (IsHoldingItem())
				{
					ResolveDropAtScreenPosition(CursorPos);
				}
			}
			else
			{
				// 原地松开（位移未超阈值）= 抓取手势的点击：拿起 / 放下
				OnSlotGrabClicked(PotentialDragSlot.Get());
			}
		}
		else
		{
			bDragging = false;
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

	// 手持有物品时点页签 = 把手持物放进该页第一个空格（设计文档：点标签按钮）
	// 页满/未激活/同页 = 无反应（保持手持），不切页
	if (IsHoldingItem())
	{
		PlaceHeldOnTab(NewTab, /*bFromDrag=*/false);
		return;
	}

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
	UE_LOG(LogTemp, Log, TEXT("[手持] 登记拖拽起点：模式=%d，页=%d，格=%d，装备槽=%d"),
	       static_cast<int32>(InSlot->GetMode()), InSlot->GetTabIndex(), InSlot->GetSlotInTab(),
	       static_cast<int32>(InSlot->GetEquipSlot()));
}

void UEOB_Widget_Inventory::OnSlotGrabClicked(UEOB_Widget_InventorySlot* InSlot)
{
	if (bDragging) return;

	UE_LOG(LogTemp, Log, TEXT("[手持] 原地点击：模式=%d，页=%d，格=%d，当前%s"),
	       static_cast<int32>(InSlot->GetMode()), InSlot->GetTabIndex(), InSlot->GetSlotInTab(),
	       IsHoldingItem() ? TEXT("手上有东西→放下") : TEXT("手上空→拿起"));

	if (IsHoldingItem())
	{
		// 抓取手势点到非法格子（红框）时保持手持（无反应），主人可换目标或右键取消
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

	// ── 装备槽：拿起 = 真正脱下（属性立即刷新），实例由本面板持有 ──
	if (InSlot->GetMode() == EEOBSlotWidgetMode::Equipment)
	{
		FEOBItemInstance PickedUp;
		if (!RefInventory->UnequipSlotToInstance(InSlot->GetEquipSlot(), PickedUp))
		{
			UE_LOG(LogTemp, Log, TEXT("[手持] 拿起失败：这个装备槽是空的"));
			return;
		}

		HeldSource = EEOBHeldSourceType::EquipmentSlot;
		HeldEquipSlot = InSlot->GetEquipSlot();
		HeldEquipmentItem = PickedUp;
		HeldGhostSlot = nullptr; // 装备来源没有幽灵格：槽位真的空了（装备面板已随广播刷新成空槽）
		BeginHeldIcon(PickedUp.Definition ? PickedUp.Definition->Icon : nullptr);

		UE_LOG(LogTemp, Log, TEXT("[手持] 已拿起装备【%s】（原始槽位=%d，属性已移除）"),
		       *PickedUp.Definition->ItemName.ToString(), static_cast<int32>(HeldEquipSlot));
		return;
	}

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

		if (TargetMode == EEOBSlotWidgetMode::Equipment)
		{
			// 背包装备 → 装备槽：能穿就穿到指定槽位（旧装备回到源背包格），不能穿就不接受（红框已提示）
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid()
				&& RefInventory->CanEquipToSlot(Item.Definition, TargetSlot->GetEquipSlot())
				&& RefInventory->EquipFromInventoryToSlot(HeldTab, HeldSlot, TargetSlot->GetEquipSlot()))
			{
				ClearHeldItem(false);
				return true;
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
			// 禁止把包放进它自己那一页——否则该页失去包裹变未激活，
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

		return false; // 装备槽不接受包裹
	}

	// ── 来源：装备槽（手上拿的是刚脱下来的装备，数据在 HeldEquipmentItem 里） ──
	if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
	{
		if (TargetMode == EEOBSlotWidgetMode::Equipment)
		{
			const EEOBEquipSlot TargetEquipSlot = TargetSlot->GetEquipSlot();

			// 点回原始槽位 = 穿回去（等于取消拿起）
			if (TargetEquipSlot == HeldEquipSlot)
			{
				ClearHeldItem(true);
				return true;
			}

			// 不匹配：抓取 = 无反应；拖拽 = 穿回原始槽位（由调用方 ClearHeldItem(true) 完成）
			if (!RefInventory->CanEquipToSlot(HeldEquipmentItem.Definition, TargetEquipSlot))
			{
				return false;
			}

			FEOBItemInstance Replaced;
			if (RefInventory->EquipInstanceToSlot(HeldEquipmentItem, TargetEquipSlot, Replaced))
			{
				if (Replaced.IsValid())
				{
					// 交换：旧装备进入抓取状态继续跟手（它的"原始槽位"就是刚脱下的这个槽）
					HeldEquipmentItem = Replaced;
					HeldEquipSlot = TargetEquipSlot;
					BeginHeldIcon(Replaced.Definition ? Replaced.Definition->Icon : nullptr);
					UE_LOG(LogTemp, Log, TEXT("[手持] 交换完成，旧装备【%s】继续跟手"),
					       *Replaced.Definition->ItemName.ToString());
				}
				else
				{
					ClearHeldItem(false); // 空槽直接穿上，手持结束
				}
				return true;
			}
			return false;
		}

		if (TargetMode == EEOBSlotWidgetMode::Inventory)
		{
			FEOBItemInstance TargetItem;
			const bool bTargetHasItem = RefInventory->GetItemAt(TargetTab, TargetSlotInTab, TargetItem)
				&& TargetItem.IsValid();

			if (!bTargetHasItem)
			{
				// 空背包格：直接放进去
				if (RefInventory->PlaceInstanceIntoSlot(HeldEquipmentItem, TargetTab, TargetSlotInTab))
				{
					ClearHeldItem(false);
					return true;
				}
				return false;
			}

			// 有货的格子：仅当格里的装备能穿到【手持装备的原始槽位】时才互换
			if (RefInventory->CanEquipToSlot(TargetItem.Definition, HeldEquipSlot))
			{
				FEOBItemInstance Displaced;
				if (RefInventory->SwapEquippedWithInventorySlot(HeldEquipSlot, TargetTab, TargetSlotInTab,
				                                                HeldEquipmentItem, Displaced))
				{
					if (Displaced.IsValid())
					{
						// 原始槽位在手持期间被别的装备占了：占用者换到手上继续手持（不吞装备）
						HeldEquipmentItem = Displaced;
						BeginHeldIcon(Displaced.Definition ? Displaced.Definition->Icon : nullptr);
						UE_LOG(LogTemp, Warning, TEXT("[手持] 原始槽位被占，【%s】换到手上继续手持"),
						       *Displaced.Definition->ItemName.ToString());
					}
					else
					{
						ClearHeldItem(false);
					}
					return true;
				}
			}
			return false; // 背包物品/穿不回去的装备 = 不接受
		}

		return false; // 包裹栏位不接受装备
	}

	return false;
}

void UEOB_Widget_Inventory::PlaceHeldOnTab(int32 TabIndex, bool bFromDrag)
{
	if (!RefInventory || !IsHoldingItem()) return;

	bool bPlaced = false;

	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		if (TabIndex != HeldTab)
		{
			bPlaced = RefInventory->MoveItemToTab(HeldTab, HeldSlot, TabIndex);
		}
	}
	else if (HeldSource == EEOBHeldSourceType::BagSlot)
	{
		if (TabIndex != HeldTab)
		{
			bPlaced = RefInventory->SwapTabs(HeldTab, TabIndex);
		}
	}
	else if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
	{
		bPlaced = RefInventory->PlaceInstanceIntoTab(HeldEquipmentItem, TabIndex);
	}

	if (bPlaced)
	{
		ClearHeldItem(false);
	}
	else if (bFromDrag)
	{
		ClearHeldItem(true); // 拖拽落到满页/未激活页/同页 = 回到原处
	}
	// 抓取点到满页/未激活页/同页 = 无反应，继续保持手持
}

void UEOB_Widget_Inventory::ResolveDropAtScreenPosition(const FVector2D& ScreenPos)
{
	if (!IsHoldingItem()) return;

	// 1. 落点是某个格子（背包格/包裹栏位/装备格都在全局注册表里）：合法就放下，非法就取消回原位
	if (UEOB_Widget_InventorySlot* TargetSlot = FindSlotAtScreenPosition(ScreenPos))
	{
		if (!PlaceHeldOnSlot(TargetSlot))
		{
			ClearHeldItem(true);
		}
		return;
	}

	// 2. 落点是页签按钮（但不是包裹栏位格子本身）= 移进该页第一个空格；失败回原位
	const int32 TabUnderCursor = FindTabIndexAtScreenPosition(ScreenPos);
	if (TabUnderCursor != INDEX_NONE)
	{
		PlaceHeldOnTab(TabUnderCursor, /*bFromDrag=*/true);
		return;
	}

	// 3. 面板外的空地：背包格/装备来源 = 丢到地上（落在任何面板的空隙上 = 无反应，走第 4 步回原位）
	if (RefInventory && !IsScreenPositionOverAnyPanel(ScreenPos))
	{
		if (HeldSource == EEOBHeldSourceType::InventorySlot)
		{
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid()
				&& RefInventory->DropItemToWorld(HeldTab, HeldSlot))
			{
				ClearHeldItem(false);
				return;
			}
		}
		else if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
		{
			if (RefInventory->DropInstanceToWorld(HeldEquipmentItem))
			{
				ClearHeldItem(false);
				return;
			}
		}
		// 包裹栏位来源：包不能丢地上，落到外面也按取消处理
	}

	// 4. 其余情况（面板空隙等）：取消手持，东西回原位
	ClearHeldItem(true);
}

UEOB_Widget_InventorySlot* UEOB_Widget_Inventory::FindSlotAtScreenPosition(const FVector2D& ScreenPos) const
{
	// 全局注册表包含所有面板的所有格子（背包格/包裹栏位/装备格），互不重叠，谁先命中都一样
	for (const TWeakObjectPtr<UEOB_Widget_InventorySlot>& SlotPtr : UEOB_Widget_InventorySlot::GetAllSlotWidgets())
	{
		UEOB_Widget_InventorySlot* SlotWidget = SlotPtr.Get();
		if (!SlotWidget) continue;
		const ESlateVisibility Vis = SlotWidget->GetVisibility();
		if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) continue;
		if (SlotWidget->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			return SlotWidget;
		}
	}
	return nullptr;
}

int32 UEOB_Widget_Inventory::FindTabIndexAtScreenPosition(const FVector2D& ScreenPos) const
{
	auto FindInBox = [&ScreenPos](UVerticalBox* Box) -> int32
	{
		if (!Box) return INDEX_NONE;
		for (UWidget* Child : Box->GetAllChildren())
		{
			if (UEOB_Widget_InventoryTab* TabWidget = Cast<UEOB_Widget_InventoryTab>(Child))
			{
				const ESlateVisibility Vis = TabWidget->GetVisibility();
				if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) continue;
				if (TabWidget->GetCachedGeometry().IsUnderLocation(ScreenPos))
				{
					return TabWidget->GetTabIndex();
				}
			}
		}
		return INDEX_NONE;
	};

	const int32 LeftResult = FindInBox(VerticalBox_TabsLeft);
	if (LeftResult != INDEX_NONE) return LeftResult;
	return FindInBox(VerticalBox_TabsRight);
}

bool UEOB_Widget_Inventory::IsScreenPositionOverAnyPanel(const FVector2D& ScreenPos) const
{
	if (GetCachedGeometry().IsUnderLocation(ScreenPos))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[手持] 落点在本背包面板范围内（含空隙）"));
		return true;
	}

	for (const TWeakObjectPtr<UWidget>& BoundsPtr : GetAllPanelBoundsWidgets())
	{
		UWidget* BoundsWidget = BoundsPtr.Get();
		if (!BoundsWidget) continue;
		const ESlateVisibility Vis = BoundsWidget->GetVisibility();
		if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) continue;
		if (BoundsWidget->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			// 丢弃被拦截时会打出是谁拦的：如果这里打出的控件尺寸明显超出面板边框，就是它挡错了
			UE_LOG(LogTemp, Log, TEXT("[手持] 落点在面板范围控件【%s】内，按“面板空隙=无反应”处理"),
			       *BoundsWidget->GetName());
			return true;
		}
	}
	return false;
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

	// 手上拿着的物品（装备来源：数据在本面板手上；背包/包裹来源：懒转移，数据仍在原格，能直接查到）
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
	else if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
	{
		HeldItem = HeldEquipmentItem;
		bGotHeld = HeldItem.IsValid();
	}
	if (!bGotHeld || !HeldItem.IsValid() || !HeldItem.Definition) return true;

	const EEOBSlotWidgetMode TargetMode = TargetSlot->GetMode();
	const bool bHeldIsBag = (HeldItem.Definition->Kind == EEOBItemKind::Bag);

	// 目标：装备槽 —— 只接受"能穿到这个槽位的装备"（背包物品/包裹栏位来源一律不接受）
	if (TargetMode == EEOBSlotWidgetMode::Equipment)
	{
		if (HeldSource == EEOBHeldSourceType::BagSlot || bHeldIsBag) return false;
		return RefInventory->CanEquipToSlot(HeldItem.Definition, TargetSlot->GetEquipSlot());
	}

	// 目标：包裹栏位
	if (TargetMode == EEOBSlotWidgetMode::BagSlot)
	{
		if (HeldSource == EEOBHeldSourceType::BagSlot) return true; // 包 ↔ 包裹栏位 = 互换页签位置，总是可以
		if (HeldSource == EEOBHeldSourceType::EquipmentSlot) return false; // 装备不能装进包裹栏位
		return bHeldIsBag; // 背包格 → 包裹栏位：只有背包物品能放上去
	}

	// 目标：背包格，来源：装备槽
	if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
	{
		FEOBItemInstance TargetItem;
		if (RefInventory->GetItemAt(TargetSlot->GetTabIndex(), TargetSlot->GetSlotInTab(), TargetItem)
			&& TargetItem.IsValid())
		{
			// 有货的格子：只有"能穿到手持装备原始槽位"的装备才接受（互换）
			return RefInventory->CanEquipToSlot(TargetItem.Definition, HeldEquipSlot);
		}
		return true; // 空格总是接受
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

// ===================== 面板外点击（点击层回调） =====================

void UEOB_Widget_Inventory::NotifyWorldLeftClickWhileHolding(const FVector2D& ScreenPos)
{
	if (!IsHoldingItem() || !RefInventory) return;

	// 落在本面板或任何已登记面板的范围控件内 = 无反应（只有真正的"面板外空地"才丢弃）
	if (IsScreenPositionOverAnyPanel(ScreenPos)) return;

	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		FEOBItemInstance Item;
		if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid()
			&& RefInventory->DropItemToWorld(HeldTab, HeldSlot))
		{
			UE_LOG(LogTemp, Log, TEXT("[手持] 面板外空地点击：已丢弃背包格来源的物品"));
			ClearHeldItem(false);
		}
	}
	else if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
	{
		if (RefInventory->DropInstanceToWorld(HeldEquipmentItem))
		{
			UE_LOG(LogTemp, Log, TEXT("[手持] 面板外空地点击：已丢弃装备来源的物品"));
			ClearHeldItem(false);
		}
	}
	// 包裹栏位来源：包不能丢地上，无反应（保持手持）
}

// ===================== 手持状态管理 =====================

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

	// 手持期间张开全屏点击层：点面板外空地 = 丢弃/取消，右键 = 取消；不会误触角色移动
	if (ClickCatcher)
	{
		ClickCatcher->SetVisibility(ESlateVisibility::Visible);
	}
}

void UEOB_Widget_Inventory::ClearHeldItem(bool bRestoreSourceVisual)
{
	if (bRestoreSourceVisual && RefInventory)
	{
		if (HeldSource == EEOBHeldSourceType::EquipmentSlot)
		{
			// 装备来源的"恢复原位" = 真正穿回原始槽位（属性重新生效）
			FEOBItemInstance Replaced;
			if (HeldEquipmentItem.IsValid()
				&& RefInventory->EquipInstanceToSlot(HeldEquipmentItem, HeldEquipSlot, Replaced))
			{
				if (Replaced.IsValid())
				{
					// 原始槽位在手持期间被别的装备占了：占用者换到手上，继续保持手持（不吞装备）
					HeldEquipmentItem = Replaced;
					BeginHeldIcon(Replaced.Definition ? Replaced.Definition->Icon : nullptr);
					UE_LOG(LogTemp, Warning, TEXT("[手持] 原始槽位被占，【%s】换到手上继续手持"),
					       *Replaced.Definition->ItemName.ToString());
					return; // 手持有新物品，不结束
				}
			}
			else if (HeldEquipmentItem.IsValid())
			{
				// 理论上不会发生（从哪个槽拿的就一定能穿回哪个槽）；兜底入包，再不行继续保持手持
				if (RefInventory->AddItem(HeldEquipmentItem) == INDEX_NONE)
				{
					UE_LOG(LogTemp, Error, TEXT("[手持] 穿回原位失败且背包已满，继续保持手持"));
					return;
				}
			}
		}
		else if (HeldGhostSlot.IsValid())
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
	}

	HeldSource = EEOBHeldSourceType::None;
	HeldTab = INDEX_NONE;
	HeldSlot = INDEX_NONE;
	HeldEquipSlot = EEOBEquipSlot::Weapon;
	HeldEquipmentItem = FEOBItemInstance();
	HeldGhostSlot = nullptr;

	if (HeldIcon)
	{
		HeldIcon->HideIcon();
	}
	if (ClickCatcher)
	{
		ClickCatcher->SetVisibility(ESlateVisibility::Collapsed);
	}
}
