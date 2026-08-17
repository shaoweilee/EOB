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
 *  - 左键松开（抓取、无捕获）：PC->GetMousePosition 是实时真值，直接定位，零误差；
 *  - 左键按住（拖拽、被按钮捕获，PC 坐标冻结）：用【自身几何体】把
 *    FSlateApplication 的实时光标（桌面坐标，不冻结）换算成图标本地坐标，
 *    与图标中心 (IconSize/2) 的差值就是本帧该补的位移：
 *        新位置 = 旧位置 + (光标本地坐标 - 中心)
 *    每帧代数精确收敛到光标，几何体即使滞后一帧也会被修正量自动抵消，
 *    不需要锚点 / 比例 / 校准偏移，也没有任何累积误差。
 *    （面板的红框高亮在拖拽中同样依赖"控件几何体 ↔ GetCursorPos"的换算且一直正确，
 *      证明这条换算在内嵌 PIE 里可靠；不可靠的只有 GetViewportWidgetGeometry。）
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

	/** 上一次设置的位置（拖拽路径里用"旧位置 + 修正量"推算新位置） */
	FVector2D LastSetPos = FVector2D::ZeroVector;

	/** 诊断用：Tick 里只打一次日志 */
	bool bLoggedFirstTick = false;

	/** 诊断用：每次 ShowIcon 后，拖拽路径只打一次修正量日志 */
	bool bLoggedDragSync = false;

	/** 每帧定位：松键用 PC 真值；按键用自身几何体修正 */
	void SyncPositionToCursor();
};
