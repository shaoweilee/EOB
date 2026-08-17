#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_HeldItemIcon.generated.h"

class UImage;

/**
 * 抓取/拖拽时"跟手"的手持物品图标。
 * 纯 C++ 控件：NativeConstruct 里自建一个 UImage 根控件，无需任何蓝图素材。
 * 自己 Tick 跟随鼠标（图标中心对准光标）；HitTestInvisible，绝不挡点击。
 * 由背包面板（EOB_Widget_Inventory）运行时创建并加到视口最高层。
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

private:
	/** 根图像控件（NativeConstruct 里自建） */
	UPROPERTY()
	TObjectPtr<UImage> Image_Icon;

	/** 图标边长（像素），ShowIcon 时更新 */
	float IconSize = 64.f;
};
