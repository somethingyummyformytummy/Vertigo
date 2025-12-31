// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ControlPanelListSlot.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UControlPanelListSlot::NativeConstruct()
{
	Super::NativeConstruct();

	SlotButton->OnReleased.AddDynamic(this, &UControlPanelListSlot::OnSlotButtonClicked);
	ensureAlwaysMsgf(SlotButton->OnReleased.IsBound(), TEXT("OnSlotButtonClicked not bound to SlotButton"));

	SlotText->SetAutoWrapText(true);
	SlotText->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);

	SetPadding(FMargin(5));
}


void UControlPanelListSlot::SetupSlotText(const int32 InSlotIndex, const FName& InSlotName)
{
	if (SlotIndexText){
		SlotIndexText->SetText(FText::AsNumber(InSlotIndex));
	}
	if (SlotText){
		SlotText->SetText(FText::FromName(InSlotName));
	}
	SlotName = InSlotName;
}

void UControlPanelListSlot::SetupSlotImageColor(const FLinearColor& InSlotColor)
{
	if (!SlotImage) return;

	SlotImage->SetColorAndOpacity(InSlotColor);
	if(InSlotColor == FLinearColor::White){
		SlotIndexText->SetColorAndOpacity(FLinearColor::Black);
		SlotText->SetColorAndOpacity(FLinearColor::Black);
	}
	else if(InSlotColor == FLinearColor::Black){
		SlotIndexText->SetColorAndOpacity(FLinearColor::White);
		SlotText->SetColorAndOpacity(FLinearColor::White);
	}
}

void UControlPanelListSlot::OnSlotButtonClicked()
{
	bool bExecuted = OnControlPanelSlotDelegate.ExecuteIfBound(SlotType, SlotName);
	ensureAlways(bExecuted);
}
