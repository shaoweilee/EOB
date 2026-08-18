#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_HoldClickCatcher.generated.h"

class UEOB_Widget_Inventory;

/**
 * 手持物品期间的全屏透明点击层（父类 UUserWidget，纯 C++ 创建，无需蓝图）：
 * - 只在"手上有东西"时显示，平时 Collapsed，完全不参与输入
 * - 垫在所有面板之下（ZOrder -100）、游戏画面之上：
 *   点到面板 = 面板自己处理；点到面板外空地 = 本层截获，转发给背包面板做"丢弃/取消"
 * - 顺带挡住手持期间对世界的左键（不会误触角色移动）和右键（= 取消手持）
 * - 双击的第二下是单独的"双击事件"，也必须吞掉，否则会穿透到世界让角色移动
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_HoldClickCatcher : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 记录手势总管（背包面板） */
	void InitCatcher(UEOB_Widget_Inventory* OwnerPanel);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply
	NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;
};
