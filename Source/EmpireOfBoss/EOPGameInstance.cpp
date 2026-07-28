// Fill out your copyright notice in the Description page of Project Settings.


#include "EOPGameInstance.h"
#include "MyGameplayTagsLibrary.h" // 引入你写 Tag 初始化的头文件

void UEOPGameInstance::Init()
{
	// 1. 极其重要：必须先调用父类的 Init()，保证引擎正常的初始化逻辑
	Super::Init();

	// 2. 在这里调用你的 Gameplay Tags 初始化
	FMyGameplayTags::InitializeNativeTags();

	// 打印一条日志，方便你在输出日志里检查是否成功调用
	UE_LOG(LogTemp, Warning, TEXT("GameInstance 自定义初始化成功：Native Gameplay Tags 已注册！"));
}

void UEOPGameInstance::AddGold(int32 Amount)
{
	GoldAmount = FMath::Max(0, GoldAmount + Amount);

	// 通知蓝图 UI 刷新金币显示
	OnGoldChanged(GoldAmount);
}
