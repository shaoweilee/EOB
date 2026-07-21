// UI 逻辑类,负责绑定和控制 Synty UI 蓝图中的具体控件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_HUDWidget.generated.h"

class UProgressBar;

UCLASS()
class EMPIREOFBOSS_API UEOB_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 🌟 核心绑定：名字必须和 Synty UI 蓝图里的血条组件名字完全一致
	// UPROPERTY(meta = (BindWidget))
	// UProgressBar* HealthBar;

public: /** 🌟 C++ 核心事件：当血量或蓝量改变时，C++ 呼叫这个函数，通知蓝图去刷新视觉效果 */
	UFUNCTION(BlueprintImplementableEvent, Category = "EOB|UI")
	void VM_UpdateHPVisual(float HealthPercent);
	UFUNCTION(BlueprintImplementableEvent, Category = "EOB|UI")
	void VM_UpdateMPVisual(float ManaPercent);

	// 敌人血条
	UFUNCTION(BlueprintImplementableEvent)
	void ShowStateBar(ESlateVisibility IsShow);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_UpdateEnemyHP(float HealthPercent);
};
