#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_DamageNumber.generated.h"

class UTextBlock;

/**
 * 伤害飘字 Widget
 * 使用方式：创建一个继承本类的 WBP_DamageNumber，
 * 里面放一个名为 DamageText 的 TextBlock（名字必须完全一致，BindWidget 会自动绑定）
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_DamageNumber : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 🌟 名字必须和蓝图里的 TextBlock 完全一致
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UTextBlock> DamageText;

public:
	/** 设置要飘的伤害数值与颜色（白=怪挨打，红=玩家挨打，橙=暴击） */
	void SetDamageValue(float Damage, FLinearColor Color, bool bIsCrit = false);

	/** M3a：飘自定义文字（如"闪避！"） */
	void SetCustomText(const FText& Text, FLinearColor Color);
};
