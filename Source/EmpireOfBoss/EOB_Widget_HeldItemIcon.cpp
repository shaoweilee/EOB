#include "EOB_Widget_HeldItemIcon.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"

void UEOB_Widget_HeldItemIcon::NativeConstruct()
{
	Super::NativeConstruct();

	// 自建一个 Image 作为根控件（本类没有蓝图子类，WidgetTree 默认是空的）
	if (!Image_Icon)
	{
		Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Icon"));
		WidgetTree->RootWidget = Image_Icon;
	}

	// 以图标中心对准鼠标位置
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));

	// 默认隐藏，拿起物品时才显示
	SetVisibility(ESlateVisibility::Collapsed);
}

void UEOB_Widget_HeldItemIcon::ShowIcon(UTexture2D* Icon, float InSize)
{
	IconSize = InSize;

	if (!IsInViewport())
	{
		AddToViewport(1000); // 画在所有面板之上
	}

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromTexture(Icon);
		Image_Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible); // 显示但不挡任何点击
}

void UEOB_Widget_HeldItemIcon::HideIcon()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UEOB_Widget_HeldItemIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetVisibility() == ESlateVisibility::Collapsed) return;

	// 跟随鼠标（配合 SetAlignmentInViewport(0.5,0.5)，图标中心 = 光标位置）
	if (APlayerController* PC = GetOwningPlayer())
	{
		float MouseX = 0.f, MouseY = 0.f;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			SetPositionInViewport(FVector2D(MouseX, MouseY));
		}
	}
}
