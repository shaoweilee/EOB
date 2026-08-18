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
 * 跟随原理（终极简版，内嵌 PIE / 独立窗口 / 任意 DPI 都无关）：
 * 格子在预览阶段吞掉左键按下，Button 不再捕获鼠标，因此
 * PC->GetMousePosition 在【抓取和拖拽全程】都是实时真值——
 * 本控件每帧直接用它定位，没有任何坐标换算，自然没有任何缩放/窗口模式的坑。
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

	/** 每帧定位：PC 鼠标位置是唯一真值源（捕获已被格子禁用，拖拽中也是实时的） */
	void SyncPositionToCursor();
};
