#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_AffixTableRow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/DataTable.h"
#include "MyGameplayTagsLibrary.h"

UEOB_InventoryComponent::UEOB_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEOB_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化固定长度背包
	Items.SetNum(InventorySize);
}

int32 UEOB_InventoryComponent::AddItem(const FEOBItemInstance& Item)
{
	if (!Item.IsValid()) return INDEX_NONE;

	const int32 EmptySlot = FindEmptySlot();
	if (EmptySlot == INDEX_NONE) return INDEX_NONE;

	Items[EmptySlot] = Item;
	OnInventoryChanged.Broadcast();
	return EmptySlot;
}

bool UEOB_InventoryComponent::RemoveItemAt(int32 SlotIndex)
{
	if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid()) return false;

	Items[SlotIndex] = FEOBItemInstance();
	OnInventoryChanged.Broadcast();
	return true;
}

bool UEOB_InventoryComponent::EquipFromInventory(int32 SlotIndex)
{
	if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid()) return false;

	FEOBItemInstance ItemToEquip = Items[SlotIndex];
	EEOBEquipSlot TargetSlot = ItemToEquip.Definition->EquipSlot;

	// 戒指自动分配：左空放左，右空放右，都满换左
	if (TargetSlot == EEOBEquipSlot::Ring)
	{
		if (!EquippedItems.Contains(EEOBEquipSlot::RingLeft)) TargetSlot = EEOBEquipSlot::RingLeft;
		else if (!EquippedItems.Contains(EEOBEquipSlot::RingRight)) TargetSlot = EEOBEquipSlot::RingRight;
		else TargetSlot = EEOBEquipSlot::RingLeft;
	}

	// 目标槽已有装备：卸下它，和背包格原位交换
	if (FEOBItemInstance* Existing = EquippedItems.Find(TargetSlot))
	{
		RemoveItemEffects(*Existing);
		Items[SlotIndex] = *Existing;
		EquippedItems.Remove(TargetSlot);
	}
	else
	{
		Items[SlotIndex] = FEOBItemInstance(); // 清空背包格
	}

	ApplyItemEffects(ItemToEquip);
	EquippedItems.Add(TargetSlot, ItemToEquip);

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[装备] 已穿上 %s"), *ItemToEquip.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::UnequipItem(EEOBEquipSlot Slot)
{
	FEOBItemInstance* Existing = EquippedItems.Find(Slot);
	if (!Existing) return false;

	const int32 EmptySlot = FindEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[装备] 背包已满，无法卸下！"));
		return false;
	}

	FEOBItemInstance Item = *Existing;
	RemoveItemEffects(Item);
	EquippedItems.Remove(Slot);
	Items[EmptySlot] = Item;

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();
	return true;
}

FEOBItemInstance UEOB_InventoryComponent::CreateItemInstance(UEOB_ItemDefinition* Def, EEOBRarity Rarity)
{
	FEOBItemInstance NewItem;
	NewItem.Definition = Def;
	NewItem.Rarity = Rarity;

	if (!Def || !AffixTable) return NewItem;

	// 1. 品质决定词缀数量（TL2 手感：白 0 / 绿 1 / 蓝 2~3 / 金 4~5）
	int32 AffixCount = 0;
	switch (Rarity)
	{
	case EEOBRarity::Green: AffixCount = 1;
		break;
	case EEOBRarity::Blue: AffixCount = FMath::RandRange(2, 3);
		break;
	case EEOBRarity::Gold: AffixCount = FMath::RandRange(4, 5);
		break;
	default: break;
	}
	if (AffixCount <= 0) return NewItem;

	// 2. 按装备槽位过滤词缀池
	static const FString ContextString(TEXT("AffixRoll"));
	TArray<FEOBAffixTableRow*> AllRows;
	AffixTable->GetAllRows<FEOBAffixTableRow>(ContextString, AllRows);

	TArray<const FEOBAffixTableRow*> Candidates;
	for (const FEOBAffixTableRow* Row : AllRows)
	{
		if (Row && Row->GEClass && Row->ApplicableSlots.Contains(Def->EquipSlot))
		{
			Candidates.Add(Row);
		}
	}

	// 3. 加权抽取，抽中即移出候选（同一件装备不出重复词缀）
	for (int32 i = 0; i < AffixCount && Candidates.Num() > 0; ++i)
	{
		float TotalWeight = 0.f;
		for (const FEOBAffixTableRow* C : Candidates) TotalWeight += FMath::Max(0.f, C->Weight);
		if (TotalWeight <= 0.f) break;

		float Roll = FMath::FRand() * TotalWeight;
		int32 Picked = 0;
		for (int32 j = 0; j < Candidates.Num(); ++j)
		{
			Roll -= FMath::Max(0.f, Candidates[j]->Weight);
			if (Roll <= 0.f)
			{
				Picked = j;
				break;
			}
		}

		const FEOBAffixTableRow* Row = Candidates[Picked];

		FEOBAffixValue Affix;
		Affix.Attribute = Row->TargetAttribute;
		Affix.ModifierOp = EGameplayModOp::Additive;
		Affix.Value = FMath::FRandRange(Row->MinValue, Row->MaxValue);
		Affix.GEClass = Row->GEClass;
		NewItem.RolledAffixes.Add(Affix);

		Candidates.RemoveAt(Picked);
	}

	return NewItem;
}

EEOBRarity UEOB_InventoryComponent::RollRarity(float WhiteWeight, float GreenWeight, float BlueWeight, float GoldWeight)
{
	const float Total = WhiteWeight + GreenWeight + BlueWeight + GoldWeight;
	if (Total <= 0.f) return EEOBRarity::White;

	float Roll = FMath::FRand() * Total;
	if ((Roll -= WhiteWeight) < 0.f) return EEOBRarity::White;
	if ((Roll -= GreenWeight) < 0.f) return EEOBRarity::Green;
	if ((Roll -= BlueWeight) < 0.f) return EEOBRarity::Blue;
	return EEOBRarity::Gold;
}

void UEOB_InventoryComponent::ApplyItemEffects(FEOBItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !Item.IsValid()) return;

	// 固定词缀 + 随机词缀一起灌
	TArray<FEOBAffixValue> AllAffixes = Item.Definition->BaseAffixes;
	AllAffixes.Append(Item.RolledAffixes);

	// 小工具：打一个 GE，数值走 SetByCaller，句柄记到装备上（卸下时统一移除）
	auto ApplyAffixGE = [&](TSubclassOf<UGameplayEffect> GEClass, float Magnitude)
	{
		if (!GEClass) return;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Item.Definition);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (!SpecHandle.IsValid()) return;

		// 🌟 把数值塞进 SetByCaller，GE 蓝图里用 Data.AffixMagnitude 读
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_AffixMagnitude, Magnitude);

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (Handle.WasSuccessfullyApplied())
		{
			Item.AppliedEffectHandles.Add(Handle);
		}
	};

	UE_LOG(LogTemp, Warning, TEXT("════ [装备] %s（%d 条词缀）════"),
	       *Item.Definition->ItemName.ToString(), AllAffixes.Num());

	for (const FEOBAffixValue& Affix : AllAffixes)
	{
		// 1. 词缀本体
		ApplyAffixGE(Affix.GEClass, Affix.Value);

		UE_LOG(LogTemp, Warning, TEXT("      词缀: %s +%.1f"), *Affix.Attribute.GetName(), Affix.Value);

		// 2. 🌟 M3a 补丁：TL2 还原——词缀加四维时，同步施加派生加成
		for (const FEOBDerivedAffixRule& Rule : DerivedAffixRules)
		{
			if (Rule.SourceAttribute == Affix.Attribute)
			{
				ApplyAffixGE(Rule.DerivedGE, Affix.Value * Rule.Multiplier);
			}
		}
	}
}

void UEOB_InventoryComponent::RemoveItemEffects(FEOBItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC) return;

	for (const FActiveGameplayEffectHandle& Handle : Item.AppliedEffectHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	Item.AppliedEffectHandles.Empty();
}

UAbilitySystemComponent* UEOB_InventoryComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

int32 UEOB_InventoryComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (!Items[i].IsValid()) return i;
	}
	return INDEX_NONE;
}
