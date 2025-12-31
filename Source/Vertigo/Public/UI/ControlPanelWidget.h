// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlPanelWidget.generated.h"

enum class EControlPanelSlotType : uint8;
class UControlPanelListSlot;
class USplineDataAsset;
class UTargetShapeDataAsset;
class UWidgetSwitcher;
class UScrollBox;
class UTextBlock;
class USlider;
class UButton;

DECLARE_DELEGATE_RetVal_OneParam(bool, FTargetMovementSignature, bool);
DECLARE_DELEGATE_OneParam(FTargetPropertySliderSignature, float);
DECLARE_DELEGATE_RetVal_OneParam(bool, FLoadSplineDataSignature, USplineDataAsset*&);
DECLARE_DELEGATE_OneParam(FTargetColorSignature, const FLinearColor&);
DECLARE_DELEGATE_OneParam(FTargetShapeSignature, const FSoftObjectPath&);

UCLASS()
class VERTIGO_API UControlPanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UControlPanelWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

public:

	FTargetMovementSignature MoveTargetDelegate;
	FTargetMovementSignature ReverseTargetDelegate;
	FTargetPropertySliderSignature TargetSpeedDelegate; 
	FTargetPropertySliderSignature TargetSizeDelegate; 
	FLoadSplineDataSignature LoadSplineDataDelegate;

	FTargetColorSignature TargetColorDelegate;
	FTargetShapeSignature TargetShapeDelegate;
	
// Settings Panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> MovementSettingsButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> TargetShapeSettingsButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> TargetColorSettingsButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> SplineCollectionSettingsButton;

// Panel 1
	//Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> MoveToggleButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> MoveToggleText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> ReverseToggleButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> ReverseToggleText;
	//Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<USlider> TargetSpeedSlider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> TargetSpeedText;
	void SyncSpeedUIWithTargetManager(float InInitialSpeed);
	
// Panel 2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
	TSoftClassPtr<UControlPanelListSlot> TargetShapeSlotClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
	TSoftClassPtr<UControlPanelListSlot> TargetColorSlotClass;
	// Target Shapes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UScrollBox> TargetShapeScrollBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> AddTargetShapeButton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<USlider> TargetSizeSlider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> TargetSizeText;
	void SyncTargetSizeUIWithTargetManager(float InInitialSize);
	// Panel 3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UScrollBox> TargetColorScrollBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UScrollBox> TargetOutlineColorScrollBox;
// Panel 4
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
	TSoftClassPtr<UControlPanelListSlot> SplineCollectionSlotClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UTextBlock> EmptyListStatusText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UScrollBox> SplineCollectionScrollBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (BindWidget))
	TObjectPtr<UButton> AddSplineDataButton; 
private:
	void ResetUIAndMovement();
	
	void BindToSlotSelected(UControlPanelListSlot*& InControlPanelListSlot);
	void OnSlotSelected(EControlPanelSlotType InSlotType, const FName& InSlotName);
	
// Settings
	UFUNCTION()
	void OnMovementSettingsButtonClicked();
	UFUNCTION()
	void OnTargetShapeSettingsButtonClicked();
	UFUNCTION()
	void OnTargetColorSettingsButtonClicked();
	UFUNCTION()
	void OnSplineCollectionSettingsButtonClicked();
// Movement
	UFUNCTION()
	void OnMoveToggleButtonClicked();
	UFUNCTION()
	void OnReverseToggleClicked();
	bool bMoveTargetEnabled = false;
	bool bReverseTargetEnabled = false; 
	UFUNCTION()
	void OnTargetSpeedSliderChanged(float InValue);
	void SetTargetSpeedUI(float InUIValue);
// Target
	//Shapes
	void PopulateTargetShapeScrollBox();
	UFUNCTION()
	void OnAddTargetShapeButtonClicked();
	void AddTargetShapeToScrollBox(UTargetShapeDataAsset* InNewAsset, int32& InSlotIndex);
	void ParseShapeCSV(const FString& InFilePath, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles);
	TMap<FName, FSoftObjectPath> CachedTargetShapes;
	UFUNCTION()
	void OnTargetSizeSliderChanged(float InValue);
	void SetTargetSizeUI(float InUIValue);

	//Colors
	void PopulateTargetColorScrollBox();
	TMap<FName, FLinearColor> CachedTargetColors;

	// Splines Collection
	void PopulateSplineCollectionScrollBox();
	void AddSplineDataToScrollBox(USplineDataAsset* InNewAsset, int32& InSlotIndex);
	UFUNCTION()
	void OnAddSplineDataButtonClicked();

	TMap<FName, TObjectPtr<USplineDataAsset>> CachedSplineAssets;

	void ParseSplineCSV(const FString& FilePath, TArray<FVector>& OutLocations, TArray<FVector>& OutTangents);

	int32 CurrentSplineSlotIndex = 1;
	int32 CurrentTargetShapeSlotIndex = 1;
};