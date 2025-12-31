// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlPanelListSlot.generated.h"

class UTextBlock;
class UButton;
class UImage;

enum class EControlPanelSlotType : uint8
{
	EYE_TARGET_COLOR,
	EYE_TARGET_SHAPE,
	SPLINE_DATA,
	UNDEFINED
};

DECLARE_DELEGATE_TwoParams(FControlPanelSlotSignature, const EControlPanelSlotType, const FName&);

UCLASS()
class VERTIGO_API UControlPanelListSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
public:

	FControlPanelSlotSignature OnControlPanelSlotDelegate;

	EControlPanelSlotType SlotType = EControlPanelSlotType::UNDEFINED;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> SlotIndexText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UImage> SlotImage;	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> SlotText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> SlotButton;

	UFUNCTION()
	void OnSlotButtonClicked();

	void SetupSlotText(const int32 InSlotIndex,const  FName& InSlotName);
	void SetupSlotImageColor(const FLinearColor& InSlotColor);
	
private:
	FName SlotName;
};
