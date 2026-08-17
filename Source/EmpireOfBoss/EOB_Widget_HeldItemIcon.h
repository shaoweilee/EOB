#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_HeldItemIcon.generated.h"

class UImage;
class USizeBox;

/**
 * 抓取/拖拽时"跟手"的手持物品图标。
 *
 * 推荐用法：建一个蓝图子类 WBP_HeldItemIcon（父类选本类），设计器里摆
 * [SizeBox_Root（宽高重载各 64）→ Image_Icon] 两层即可，然后在 WBP_Inventory
 * 的类默认值里把 HeldIconWidgetClass 指到 WBP_HeldItemIcon。
 *
 * 纯 C++ 兜底：若直接用 C++ 类创建（Image_Icon 没绑定），NativeConstruct 会自建
 * [SizeBox → Image] 结构，功能相同。
 *
 * 自己 Tick 跟随鼠标（图标中心对准光标）；HitTestInvisible，绝不挡点击。
 * 由背包面板（EOB_Widget_Inventory）运行时创建并加到视口最高层。
 *
 * 跟随坐标的门道（编辑器内嵌视口 / 独立窗口 / 任意 DPI 缩放都适配）：
 *  - 左键松开时：PC->GetMousePosition 是实时真值（与 SetPositionInViewport 同空间），
 *    直接用它，并顺手把锚点刷新成最新；
 *  - 左键按住时（拖拽）：PC 坐标被按钮捕获冻结，改用
 *    "锚点 + (Slate 实时光标 - 锚点光标) / 图标自身几何缩放" 推算——
 *    几何缩放取自本控件 GetCachedGeometry().GetAbsoluteScale()，
 *    随内嵌视口拉伸实时变化，永远匹配。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_HeldItemIcon : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 显示图标（InSize = 边长，像素）。未加入视口时会自动加入最高层 */
	void ShowIcon(UTexture2D* Icon, float InSize);

	/** 隐藏（放下/取消手持时调用） */
	void HideIcon();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 根尺寸框（WBP 里按这个名字摆 SizeBox；纯 C++ 时自建） */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_Root;

	/** 图标图像（WBP 里按这个名字摆 Image，作为 SizeBox_Root 的子控件；纯 C++ 时自建） */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

private:
	/** 图标边长（像素），ShowIcon 时更新 */
	float IconSize = 64.f;

	/** 诊断用：Tick 里只打一次日志 */
	bool bLoggedFirstTick = false;

	/** 锚点：PC 坐标系下的鼠标位置（SetPositionInViewport 期望的空间） */
	FVector2D AnchorViewportPos = FVector2D::ZeroVector;

	/** 锚点：同一时刻的 Slate 实时光标位置（桌面坐标） */
	FVector2D AnchorDesktopPos = FVector2D::ZeroVector;

	/** 锚点是否有效（拿到过 PC 坐标即为 true） */
	bool bAnchorValid = false;

	/** 记录锚点（拿起那一刻调用；拖拽中 PC 坐标冻结在按下位置，误差 ≤ 拖拽阈值，可接受） */
	void CaptureAnchor();

	/** 把图标摆到鼠标当前位置（松键用真值，按禁用锚点推算） */
	void SyncPositionToCursor();
};
