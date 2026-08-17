#include "EOB_Widget_Inventory.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
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

	// ── 手持物品图标：纯 C++ 运行时创建，无需任何蓝图素材 ──
	// 加到视口最高层，跟着鼠标走；不可命中测试，绝不挡点击。
	HeldIconWidget = CreateWidget<UUserWidget>(GetOwningPlayer());
	if (HeldIconWidget && HeldIconWidget->WidgetTree)
	{
		HeldIconImage = NewObject<UImage>(HeldIconWidget->WidgetTree);
		HeldIconWidget->WidgetTree->RootWidget = HeldIconImage;
		HeldIconWidget->AddToViewport(1000);
		HeldIconWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f)); // 图标中心对准鼠标
		HeldIconWidget->SetVisibility(ESlateVisibility::Collapsed); // 拿起时才显示
	}

	RefreshUI();
}

void UEOB_Widget_Inventory::NativeDestruct()
{
	if (HeldIconWidget)
	{
		HeldIconWidget->RemoveFromParent();
		HeldIconWidget = nullptr;
		HeldIconImage = nullptr;
	}
	Super::NativeDestruct();
}

void UEOB_Widget_Inventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FSlateApplication::IsInitialized()) return;

	const bool bLeftDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);
	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	// 手持图标跟着鼠标走
	if (HeldIconWidget && HeldIconWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			float MouseX = 0.f, MouseY = 0.f;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				HeldIconWidget->SetPositionInViewport(FVector2D(MouseX, MouseY));
			}
		}
	}

	// 按下后位移超过阈值：进入拖拽；手上还没东西就从起点格子拿起来
	if (PotentialDragSlot.IsValid() && bLeftDown && !bDragging
		&& FVector2D::Distance(CursorPos, PotentialDragPos) >= DragThresholdPixels)
	{
		bDragging = true;
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
	CloseAllTabDropdowns(); // 切页时收起所有"偏好"下拉面板
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

			SlotWidget->InitInventorySlot(RefInventory, CurrentTabIndex, i, this);
			SlotWidget->UpdateSlot(Item);
			GridPanel_Items->AddChildToUniformGrid(SlotWidget, i / GridColumns, i % GridColumns);
		}
	}

	// 页签状态跟着刷新（激活/禁用、当前页高亮、偏好图标、包裹栏位）
	RefreshTabs();
}

// ===================== 抓取 / 拖拽 =====================

void UEOB_Widget_Inventory::NotifySlotLeftPressed(UEOB_Widget_InventorySlot* InventorySlot, const FVector2D& ScreenPos)
{
	PotentialDragSlot = InventorySlot;
	PotentialDragPos = ScreenPos;
}

void UEOB_Widget_Inventory::OnSlotGrabClicked(UEOB_Widget_InventorySlot* InventorySlot)
{
	// 拖拽中松开会触发起点按钮的 OnClicked（UMG 按钮默认 DownAndUp，松手即触发），
	// 这次"点击"必须忽略——拖拽的落点由 Tick 里的 ResolveDropAtScreenPosition 结算。
	if (bDragging) return;

	if (IsHoldingItem())
	{
		PlaceHeldOnSlot(InventorySlot); // 手上已有物品：这次点击 = 放下
	}
	else
	{
		TryPickUpFromSlot(InventorySlot); // 手上没有：这次点击 = 拿起
	}
}

void UEOB_Widget_Inventory::CancelHeldItem()
{
	ClearHeldItem(true);
}

void UEOB_Widget_Inventory::TryPickUpFromSlot(UEOB_Widget_InventorySlot* InventorySlot)
{
	if (!RefInventory || !InventorySlot || IsHoldingItem()) return;

	FEOBItemInstance Item;
	EEOBHeldSourceType SourceType = EEOBHeldSourceType::None;

	if (InventorySlot->GetMode() == EEOBSlotWidgetMode::Inventory)
	{
		if (RefInventory->GetItemAt(InventorySlot->GetTabIndex(), InventorySlot->GetSlotInTab(), Item) && Item.IsValid())
		{
			SourceType = EEOBHeldSourceType::InventorySlot;
			HeldTab = InventorySlot->GetTabIndex();
			HeldSlot = InventorySlot->GetSlotInTab();
		}
	}
	else if (InventorySlot->GetMode() == EEOBSlotWidgetMode::BagSlot)
	{
		if (RefInventory->GetTabBag(InventorySlot->GetTabIndex(), Item) && Item.IsValid())
		{
			SourceType = EEOBHeldSourceType::BagSlot;
			HeldTab = InventorySlot->GetTabIndex();
			HeldSlot = INDEX_NONE;
		}
	}

	if (SourceType == EEOBHeldSourceType::None || !Item.Definition) return;

	// 注意：东西仍在原格（数据不动），只是源格显示为空 + 图标跟手。
	// 放下时调用组件的移动/换包函数一次性完成，中途取消零成本、零风险。
	HeldSource = SourceType;
	HeldGhostSlot = InventorySlot;
	InventorySlot->UpdateSlot(FEOBItemInstance());
	BeginHeldIcon(Item.Definition->Icon);
}

void UEOB_Widget_Inventory::PlaceHeldOnSlot(UEOB_Widget_InventorySlot* TargetSlot)
{
	if (!RefInventory || !TargetSlot || !IsHoldingItem()) return;

	const EEOBSlotWidgetMode TargetMode = TargetSlot->GetMode();
	const int32 TargetTab = TargetSlot->GetTabIndex();
	const int32 TargetSlotInTab = TargetSlot->GetSlotInTab();

	// ── 来源：背包格 ──
	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		if (TargetMode == EEOBSlotWidgetMode::Inventory)
		{
			// 放回同一格 = 放下不动
			if (TargetTab == HeldTab && TargetSlotInTab == HeldSlot)
			{
				ClearHeldItem(true);
				return;
			}
			// 目标格为空 = 移动；有货 = 互换
			if (RefInventory->MoveOrSwapItem(HeldTab, HeldSlot, TargetTab, TargetSlotInTab))
			{
				ClearHeldItem(false);
			}
			// 失败：继续拿着（右键可取消）
			return;
		}

		if (TargetMode == EEOBSlotWidgetMode::BagSlot)
		{
			// 包裹栏位只接受"背包物品"：空栏位 = 装备到该页；有包 = 换包（组件内部做容量检查）
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid()
				&& Item.Definition && Item.Definition->Kind == EEOBItemKind::Bag)
			{
				if (RefInventory->EquipBagFromSlotToTab(HeldTab, HeldSlot, TargetTab))
				{
					ClearHeldItem(false);
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[背包 UI] 包裹栏位只能放背包物品。"));
			}
			return;
		}

		// 目标是装备面板的格子：下一阶段才支持，本次不动
		return;
	}

	// ── 来源：包裹栏位（拿的是装备的背包） ──
	if (HeldSource == EEOBHeldSourceType::BagSlot)
	{
		if (TargetMode == EEOBSlotWidgetMode::BagSlot)
		{
			if (TargetTab == HeldTab)
			{
				ClearHeldItem(true); // 放回原栏位
				return;
			}
			// 两个包裹栏位互换位置（连带包内物品；目标为空 = 挪过去）
			if (RefInventory->SwapTabs(HeldTab, TargetTab))
			{
				ClearHeldItem(false);
			}
			return;
		}

		if (TargetMode == EEOBSlotWidgetMode::Inventory)
		{
			// 把装备的背包卸到指定物品格（= 取消装备包裹；包内有物品时组件会拒绝）
			if (RefInventory->UnequipBagToSlot(HeldTab, TargetTab, TargetSlotInTab))
			{
				ClearHeldItem(false);
			}
			return;
		}
	}
}

void UEOB_Widget_Inventory::ResolveDropAtScreenPosition(const FVector2D& ScreenPos)
{
	if (!IsHoldingItem()) return;

	// 落在某个格子上：走正常放下逻辑
	if (UEOB_Widget_InventorySlot* TargetSlot = FindSlotAtScreenPosition(ScreenPos))
	{
		PlaceHeldOnSlot(TargetSlot);
		return;
	}

	// 没落在任何格子上
	if (HeldSource == EEOBHeldSourceType::InventorySlot)
	{
		// 拖出物品栏面板范围 = 丢到地上（保留品质和词缀）；还在面板内 = 放回原处
		const bool bOutsidePanel = !GetCachedGeometry().IsUnderLocation(ScreenPos);
		if (bOutsidePanel && RefInventory)
		{
			FEOBItemInstance Item;
			if (RefInventory->GetItemAt(HeldTab, HeldSlot, Item) && Item.IsValid())
			{
				if (RefInventory->DropItemToWorld(HeldTab, HeldSlot))
				{
					ClearHeldItem(false); // 已经丢出去了（刷新会重建格子，无需还原源格）
					return;
				}
			}
		}
		ClearHeldItem(true); // 面板内松手 / 丢弃失败：放回原处
		return;
	}

	// 包裹栏位上拿起来的背包：落在空处 = 取消（装备的背包不允许丢到地上）
	ClearHeldItem(true);
}

UEOB_Widget_InventorySlot* UEOB_Widget_Inventory::FindSlotAtScreenPosition(const FVector2D& ScreenPos) const
{
	auto IsSlotUnderCursor = [&ScreenPos](UEOB_Widget_InventorySlot* InventorySlot) -> bool
	{
		if (!InventorySlot) return false;
		const ESlateVisibility Vis = InventorySlot->GetVisibility();
		if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) return false;
		return InventorySlot->GetCachedGeometry().IsUnderLocation(ScreenPos);
	};

	// 12 个包裹栏位（页签左侧的 Slot_Bag）
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

	// 当前页的物品格子
	if (GridPanel_Items)
	{
		for (UWidget* Child : GridPanel_Items->GetAllChildren())
		{
			if (UEOB_Widget_InventorySlot* InventorySlot = Cast<UEOB_Widget_InventorySlot>(Child))
			{
				if (IsSlotUnderCursor(InventorySlot)) return InventorySlot;
			}
		}
	}

	return nullptr;
}

void UEOB_Widget_Inventory::BeginHeldIcon(UTexture2D* Icon)
{
	if (!HeldIconWidget || !HeldIconImage) return;

	HeldIconImage->SetBrushFromTexture(Icon);
	HeldIconImage->SetDesiredSizeOverride(FVector2D(HeldIconSize, HeldIconSize));
	HeldIconWidget->SetVisibility(ESlateVisibility::HitTestInvisible); // 显示但不挡点击
}

void UEOB_Widget_Inventory::ClearHeldItem(bool bRestoreSourceVisual)
{
	// 还原源格显示（取消手持/放回原处时；成功放下后格子已被刷新重建，弱引用自动失效）
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

	if (HeldIconWidget)
	{
		HeldIconWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
