#include "EOB_Widget_HeldItemIcon.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
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

	// 显式定死宽高（WBP 里已在设计器设过 64，这里按 IconSize 再覆写一遍，两边一致）
	if (SizeBox_Root)
	{
		SizeBox_Root->SetWidthOverride(IconSize);
		SizeBox_Root->SetHeightOverride(IconSize);
	}
	else if (Image_Icon)
	{
		Image_Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible); // 显示但不挡任何点击

	// 立刻把图标摆到鼠标当前位置（不用等下一帧 Tick）
	SyncPositionToCursor();

	UE_LOG(LogTemp, Log, TEXT("[手持] ShowIcon：贴图=%s，边长=%.0f，常数偏移=(%.1f, %.1f)"),
	       Icon ? TEXT("有效") : TEXT("空！"),
	       IconSize,
	       ConvertOffset.X, ConvertOffset.Y);
}

void UEOB_Widget_HeldItemIcon::HideIcon()
{
	SetVisibility(ESlateVisibility::Collapsed);
	UE_LOG(LogTemp, Log, TEXT("[手持] 图标已隐藏"));
}

void UEOB_Widget_HeldItemIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 即使图标隐藏（Collapsed）也持续运行：松键时持续校准常数偏移，拿起那一刻就是准的。
	SyncPositionToCursor();

	if (GetVisibility() == ESlateVisibility::Collapsed) return;

	// 只打第一帧，确认 Tick 在跑
	if (!bLoggedFirstTick)
	{
		bLoggedFirstTick = true;
		UE_LOG(LogTemp, Log, TEXT("[手持] Tick 跟随中"));
	}
}

void UEOB_Widget_HeldItemIcon::SyncPositionToCursor()
{
	if (!FSlateApplication::IsInitialized()) return;

	// 实时光标（桌面坐标）——拖拽中被按钮捕获也不会冻结
	const FVector2D LiveDesktop = FSlateApplication::Get().GetCursorPos();

	// 视口控件几何体：AbsoluteToLocal 一次性完成 桌面坐标 → SetPositionInViewport 空间 的换算。
	// Windows 显示缩放、编辑器应用程序缩放、内嵌视口 UI 缩放全部包含在这个几何变换里。
	const FGeometry ViewportGeo = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
	const FVector2D Converted = ViewportGeo.AbsoluteToLocal(LiveDesktop);

	const bool bLeftDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);

	// ── 左键松开（抓取状态、无捕获）：PC 坐标是实时真值 ──
	// 直接用真值定位（零误差），并校准常数偏移（真值 - 换算值）。
	if (!bLeftDown)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			float MouseX = 0.f, MouseY = 0.f;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				const FVector2D Truth(MouseX, MouseY);
				ConvertOffset = Truth - Converted;
				SetPositionInViewport(Truth);
				return;
			}
		}
	}

	// ── 左键按住（拖拽中、被捕获）：PC 坐标冻结，用换算值 + 校准过的常数偏移 ──
	SetPositionInViewport(Converted + ConvertOffset);
}
