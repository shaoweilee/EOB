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
 * 跟随原理（编辑器内嵌视口 / 独立窗口 / 任意 DPI、任意缩放叠加都自适应）：
 *  - FSlateApplication::GetCursorPos() 拿实时光标（桌面坐标，拖拽中被捕获也不冻结）；
 *  - 用视口控件几何体 AbsoluteToLocal() 一次性换算成 SetPositionInViewport 的空间，
 *    Windows 缩放 × 编辑器应用程序缩放 × 视口 UI 缩放全部包含在该几何变换里；
 *  - 左键松开时，PC->GetMousePosition 是实时真值，用它定位并把
 *    "真值 - 换算值"记为常数偏移；左键按住时用"换算值 + 常数偏移"，
 *    即使换算存在固定原点差也能自动补平。
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

	/** 常数偏移：松键时用 PC 真值校准（PC真值 - 几何换算值），按键时补到换算值上 */
	FVector2D ConvertOffset = FVector2D::ZeroVector;

	/** 每帧定位：几何换算 + 松键真值校准 */
	void SyncPositionToCursor();
};
