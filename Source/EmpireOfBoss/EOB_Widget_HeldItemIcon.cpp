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
	bLoggedDragSync = false;

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

	UE_LOG(LogTemp, Log, TEXT("[手持] ShowIcon：贴图=%s，边长=%.0f，位置=(%.0f, %.0f)"),
	       Icon ? TEXT("有效") : TEXT("空！"),
	       IconSize,
	       LastSetPos.X, LastSetPos.Y);
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
	if (!FSlateApplication::IsInitialized()) return;

	const bool bLeftDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);

	// ── 左键松开（抓取状态、无捕获）：PC 坐标是实时真值，直接定位，零误差 ──
	if (!bLeftDown)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			float MouseX = 0.f, MouseY = 0.f;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				LastSetPos = FVector2D(MouseX, MouseY);
				SetPositionInViewport(LastSetPos);
				return;
			}
		}
		return;
	}

	// ── 左键按住（拖拽中、被按钮捕获，PC 坐标冻结）──
	// 用【自身几何体】把实时光标（桌面坐标，不冻结）换算成图标本地坐标。
	// 光标应位于图标中心 (IconSize/2, IconSize/2)，差多少补多少：
	//     新位置 = 旧位置 + (光标本地坐标 - 中心)
	// 每帧代数精确收敛：即使几何体滞后一帧，修正量也会把偏差抵消，无累积误差。
	const FGeometry& MyGeo = GetCachedGeometry();
	if (MyGeo.GetLocalSize().X < 1.f)
	{
		return; // 几何体还没就绪（刚显示的第一帧），等下一帧
	}

	const FVector2D LiveDesktop = FSlateApplication::Get().GetCursorPos();
	const FVector2D IconLocal = MyGeo.AbsoluteToLocal(LiveDesktop);
	const FVector2D Center(IconSize * 0.5f, IconSize * 0.5f);
	const FVector2D Correction = IconLocal - Center;

	LastSetPos += Correction;
	SetPositionInViewport(LastSetPos);

	// 诊断：每次拿起后只打一次，看修正量是否合理（正常应只有几个像素）
	if (!bLoggedDragSync)
	{
		bLoggedDragSync = true;
		UE_LOG(LogTemp, Log, TEXT("[手持] 拖拽定位：光标在图标内=(%.1f, %.1f)，修正=(%.1f, %.1f)"),
		       IconLocal.X, IconLocal.Y, Correction.X, Correction.Y);
	}
}
