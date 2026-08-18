#include "EOB_Widget_HeldItemIcon.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"

void UEOB_Widget_HeldItemIcon::NativeConstruct()
{
	Super::NativeConstruct();

	// 纯 C++ 兜底：没有蓝图绑定（直接用 C++ 类创建）时，自建 [SizeBox → Image]。
	// 正常用法是 WBP_HeldItemIcon 在设计器里摆好 SizeBox_Root / Image_Icon，走绑定分支，不进这里。
	if (!Image_Icon && WidgetTree)
	{
		SizeBox_Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Root"));
		WidgetTree->RootWidget = SizeBox_Root;

		Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Icon"));
		SizeBox_Root->AddChild(Image_Icon);
	}

	// 以图标中心对准鼠标位置
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));

	// 默认隐藏，拿起物品时才显示
	SetVisibility(ESlateVisibility::Collapsed);

	UE_LOG(LogTemp, Log, TEXT("[手持] 图标控件已构造（SizeBox=%s，Image=%s）"),
	       SizeBox_Root ? TEXT("OK") : TEXT("未绑定/未建"),
	       Image_Icon ? TEXT("OK") : TEXT("未绑定/未建"));
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
	}

	// 定死边长的三道保险：WBP 设计器里一道，这里两道——
	// SetDesiredSizeInViewport 直接锁根控件在视口层里的期望尺寸，不管 WBP 内部结构如何都生效。
	if (SizeBox_Root)
	{
		SizeBox_Root->SetWidthOverride(IconSize);
		SizeBox_Root->SetHeightOverride(IconSize);
	}
	else if (Image_Icon)
	{
		Image_Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}
	SetDesiredSizeInViewport(FVector2D(IconSize, IconSize));

	SetVisibility(ESlateVisibility::HitTestInvisible); // 显示但不挡任何点击

	// 立刻把图标摆到鼠标当前位置（不用等下一帧 Tick）
	SyncPositionToCursor();

	UE_LOG(LogTemp, Log, TEXT("[手持] ShowIcon：贴图=%s，边长=%.0f"),
	       Icon ? TEXT("有效") : TEXT("空！"), IconSize);
}

void UEOB_Widget_HeldItemIcon::HideIcon()
{
	SetVisibility(ESlateVisibility::Collapsed);
	UE_LOG(LogTemp, Log, TEXT("[手持] 图标已隐藏"));
}

void UEOB_Widget_HeldItemIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetVisibility() == ESlateVisibility::Collapsed) return;

	SyncPositionToCursor();

	// 只打第一帧，确认 Tick 在跑
	if (!bLoggedFirstTick)
	{
		bLoggedFirstTick = true;
		UE_LOG(LogTemp, Log, TEXT("[手持] Tick 跟随中"));
	}
}

void UEOB_Widget_HeldItemIcon::SyncPositionToCursor()
{
	// 格子的预览阶段已吞掉左键按下，Button 不再捕获鼠标——
	// PC->GetMousePosition 在抓取和拖拽全程都是实时真值，直接定位，零换算。
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	float MouseX = 0.f, MouseY = 0.f;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		SetPositionInViewport(FVector2D(MouseX, MouseY));
	}
}
