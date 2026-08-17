#include "EOB_Widget_HeldItemIcon.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
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

	// 校准两套坐标系的差值，然后立刻把图标摆到鼠标当前位置（不用等下一帧 Tick）
	CalibrateCursorSpaceOffset();
	SyncPositionToCursor();

	UE_LOG(LogTemp, Log, TEXT("[手持] ShowIcon：贴图=%s，边长=%.0f，坐标差值=(%.0f, %.0f)"),
	       Icon ? TEXT("有效") : TEXT("空！"),
	       IconSize,
	       CursorSpaceOffset.X, CursorSpaceOffset.Y);
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

	// 跟随鼠标（配合 SetAlignmentInViewport(0.5,0.5)，图标中心 = 光标位置）
	SyncPositionToCursor();

	// 只打第一帧，确认 Tick 在跑
	if (!bLoggedFirstTick)
	{
		bLoggedFirstTick = true;
		UE_LOG(LogTemp, Log, TEXT("[手持] Tick 跟随中"));
	}
}

void UEOB_Widget_HeldItemIcon::CalibrateCursorSpaceOffset()
{
	// PC->GetMousePosition 的坐标空间与 SetPositionInViewport 完全对齐（抓取模式已验证对准），
	// 但左键按住被按钮捕获时它会冻结；FSlateApplication::GetCursorPos 永远实时但坐标系不同。
	// 两者的差是固定值（窗口原点等因素），在这里一次性校准，之后每帧用实时光标 + 差值即可。
	CursorSpaceOffset = FVector2D::ZeroVector;

	if (!FSlateApplication::IsInitialized()) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	float MouseX = 0.f, MouseY = 0.f;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		CursorSpaceOffset = FVector2D(MouseX, MouseY) - FSlateApplication::Get().GetCursorPos();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[手持] 校准时取 PC 鼠标坐标失败，差值按 (0,0) 处理"));
	}
}

void UEOB_Widget_HeldItemIcon::SyncPositionToCursor()
{
	if (!FSlateApplication::IsInitialized()) return;

	// 实时光标（桌面坐标）+ 校准差值 = SetPositionInViewport 期望的坐标
	const FVector2D LiveCursor = FSlateApplication::Get().GetCursorPos();
	SetPositionInViewport(LiveCursor + CursorSpaceOffset);
}
