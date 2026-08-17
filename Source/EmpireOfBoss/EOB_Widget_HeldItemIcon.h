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
 * 跟随坐标的门道（两套坐标系各有一个毛病，取长补短）：
 *  - PC->GetMousePosition：坐标空间和 SetPositionInViewport 完全对齐（抓取模式验证过），
 *    但左键按住被按钮捕获时，它读的视口缓存会冻结在按下位置；
 *  - FSlateApplication::GetCursorPos：永远实时（桌面坐标），但空间和视口坐标系不同。
 *  做法：ShowIcon 时两者各取一次算出差值（CursorSpaceOffset），之后每帧用
 *  实时光标 + 差值 得到正确视口坐标。
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

	/** PC 鼠标坐标系 与 Slate 桌面坐标系 的固定差值（ShowIcon 时校准） */
	FVector2D CursorSpaceOffset = FVector2D::ZeroVector;

	/** 用 PC 坐标（正确空间）与 Slate 实时光标（桌面坐标）算出固定差值 */
	void CalibrateCursorSpaceOffset();

	/** 立刻把图标摆到鼠标当前位置（实时光标 + 校准差值） */
	void SyncPositionToCursor();
};
