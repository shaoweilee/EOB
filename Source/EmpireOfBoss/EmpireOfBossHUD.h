// HUD 治理类,负责管理 UI 的生命周期，将其生成并推上屏幕。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EmpireOfBossHUD.generated.h"

class UEOB_HUDWidget;

UCLASS()
class EMPIREOFBOSS_API AEmpireOfBossHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// 🌟 暴露给 HUD 蓝图的资产槽位
	UPROPERTY(EditAnywhere, Category = "EOB_UI")
	TSubclassOf<UEOB_HUDWidget> HUDWidgetClass;

private:
	// 运行时生成的 UI 实例指针
	UPROPERTY()
	UEOB_HUDWidget* HUDWidgetInstance;

public:
	// 🌟 新增：公开的UI初始化方法，由Subsystem统一调用
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void InitializeHUDWidgets();

	// 🌟 公开接口：方便外部随时获取 UI 实例
	FORCEINLINE UEOB_HUDWidget* GetHUDWidgetInstance() const { return HUDWidgetInstance; }
};
