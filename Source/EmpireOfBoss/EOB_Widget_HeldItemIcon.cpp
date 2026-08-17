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

	// 记录锚点，然后立刻把图标摆到鼠标当前位置（不用等下一帧 Tick）
	CaptureAnchor();
	SyncPositionToCursor();

	UE_LOG(LogTemp, Log, TEXT("[手持] ShowIcon：贴图=%s，边长=%.0f，锚点=%s (%.0f, %.0f)"),
	       Icon ? TEXT("有效") : TEXT("空！"),
	       IconSize,
	       bAnchorValid ? TEXT("有效") : TEXT("无效"),
	       AnchorViewportPos.X, AnchorViewportPos.Y);
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

void UEOB_Widget_HeldItemIcon::CaptureAnchor()
{
	bAnchorValid = false;

	if (!FSlateApplication::IsInitialized()) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		UE_LOG(LogTemp, Warning, TEXT("[手持] 记录锚点时取 PC 鼠标坐标失败"));
		return;
	}

	AnchorViewportPos = FVector2D(MouseX, MouseY);
	AnchorDesktopPos = FSlateApplication::Get().GetCursorPos();
	bAnchorValid = true;
}

void UEOB_Widget_HeldItemIcon::SyncPositionToCursor()
{
	if (!FSlateApplication::IsInitialized()) return;

	const FVector2D LiveDesktop = FSlateApplication::Get().GetCursorPos();
	const bool bLeftDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);

	// ── 左键松开（抓取状态、无捕获）：PC 坐标是实时真值，直接用，并顺手刷新锚点 ──
	// 这样锚点在抓取期间每帧都是新的；即使随后按住左键开始拖拽，推算的起点也是零误差的。
	if (!bLeftDown)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			float MouseX = 0.f, MouseY = 0.f;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				AnchorViewportPos = FVector2D(MouseX, MouseY);
				AnchorDesktopPos = LiveDesktop;
				bAnchorValid = true;

				SetPositionInViewport(AnchorViewportPos);
				return;
			}
		}
	}

	// ── 左键按住（拖拽中、被按钮捕获）：PC 坐标冻结，用锚点 + 实时位移推算 ──
	// 缩放取"本控件几何的绝对缩放"——它等于内嵌视口拉伸比 × 界面缩放，
	// 随视口拖大拖小实时变化，永远匹配（不能用 GetViewportScale，那取的是别的东西）。
	if (bAnchorValid)
	{
		float LiveScale = GetCachedGeometry().GetAccumulatedLayoutTransform().GetScale();
		if (LiveScale < KINDA_SMALL_NUMBER)
		{
			LiveScale = 1.f;
		}

		const FVector2D ViewportPos = AnchorViewportPos + (LiveDesktop - AnchorDesktopPos) / LiveScale;
		SetPositionInViewport(ViewportPos);
	}
	else
	{
		// 兜底：从没拿到过锚点（理论上不会发生），直写桌面坐标，至少跟随是实时的
		SetPositionInViewport(LiveDesktop);
	}
}
