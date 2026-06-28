#include "EmpireOfBossHUD.h"
#include "EOB_HUDWidget.h"
#include "Blueprint/UserWidget.h"

void AEmpireOfBossHUD::BeginPlay()
{
	Super::BeginPlay();
	// 🌟 删掉原来的自动创建逻辑，改由Subsystem调用InitializeHUDWidgets
	// 保证属性先初始化，UI后创建，时序完全可控
}

// 🌟 新增：把原来的UI创建逻辑移到这里
void AEmpireOfBossHUD::InitializeHUDWidgets()
{
	// 防止重复创建
	if (HUDWidgetInstance) return;

	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UEOB_HUDWidget>(GetOwningPlayerController(), HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
		}
	}
}
