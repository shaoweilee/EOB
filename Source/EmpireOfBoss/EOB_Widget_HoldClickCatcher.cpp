#include "EOB_Widget_HoldClickCatcher.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "EOB_Widget_Inventory.h"

void UEOB_Widget_HoldClickCatcher::InitCatcher(UEOB_Widget_Inventory* OwnerPanel)
{
	RefPanel = OwnerPanel;
}

void UEOB_Widget_HoldClickCatcher::NativeConstruct()
{
	Super::NativeConstruct();

	// 纯 C++ 控件没有蓝图根：自建一张全透明图片当全屏点击层（只挡点击，不挡画面）
	if (!GetRootWidget())
	{
		UImage* RootImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RootBlocker"));
		RootImage->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
		WidgetTree->RootWidget = RootImage;
	}

	// 尺寸给足：任何分辨率/窗口模式/DPI 缩放都能盖住整个视口
	SetDesiredSizeInViewport(FVector2D(16384.f, 16384.f));
}

FReply UEOB_Widget_HoldClickCatcher::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                             const FPointerEvent& InMouseEvent)
{
	// 能点到这里 = 落点不在任何面板（格子/页签/面板背景都在本层之上，会先收到事件）
	if (RefPanel.IsValid())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			// 任何时刻右键 = 取消手持（装备来源 = 穿回原始槽位）
			RefPanel->CancelHeldItem();
			return FReply::Handled();
		}

		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			// 面板外空地左键 = 按文档规则丢弃（面板内部会再复核一遍"是不是落在面板空隙"）
			RefPanel->NotifyWorldLeftClickWhileHolding(InMouseEvent.GetScreenSpacePosition());
			return FReply::Handled();
		}
	}

	// 手持期间屏蔽其余按键对世界的穿透
	return FReply::Handled();
}
