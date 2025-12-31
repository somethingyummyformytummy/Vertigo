// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ControlPanelWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/ControlPanelListSlot.h"
#include "Utils/SplineDataAsset.h"
#include "Utils/TargetPropertiesDataAsset.h"
#include "Utils/TargetShapeDataAsset.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "Editor.h"
#include "UObject/SavePackage.h"
#endif

UControlPanelWidget::UControlPanelWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UControlPanelListSlot> TempSlotClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/Blueprints/UI/WBP_ControlPanelLongSlot.WBP_ControlPanelLongSlot_C'"));
	if (TempSlotClass.Succeeded()){
		SplineCollectionSlotClass = TempSlotClass.Class;
	}
	ConstructorHelpers::FClassFinder<UControlPanelListSlot> TempSmallSlotClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/Blueprints/UI/WBP_ControlPanelSmallSlot.WBP_ControlPanelSmallSlot_C'"));
	if (TempSmallSlotClass.Succeeded()){
		TargetColorSlotClass = TempSmallSlotClass.Class;
		TargetShapeSlotClass = TempSmallSlotClass.Class;
	}
}

void UControlPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Button Binds
	MovementSettingsButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnMovementSettingsButtonClicked);
	ensureAlwaysMsgf(MovementSettingsButton->OnReleased.IsBound(), TEXT("OnMovementSettingsButtonClicked not bound to MovementSettingsButton"));
	TargetShapeSettingsButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnTargetShapeSettingsButtonClicked);
	ensureAlwaysMsgf(TargetShapeSettingsButton->OnReleased.IsBound(), TEXT("OnTargetShapeSettingsButtonClicked not bound to TargetShapeSettingsButton"));
	TargetColorSettingsButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnTargetColorSettingsButtonClicked);
	ensureAlwaysMsgf(TargetColorSettingsButton->OnReleased.IsBound(), TEXT("OnTargetColorSettingsButtonClicked not bound to TargetColorSettingsButton"));
	SplineCollectionSettingsButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnSplineCollectionSettingsButtonClicked);
	ensureAlwaysMsgf(SplineCollectionSettingsButton->OnReleased.IsBound(), TEXT("OnSplineCollectionSettingsButtonClicked not bound to SplineCollectionSettingsButton"));
	// Settings
	MoveToggleButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnMoveToggleButtonClicked);
	ensureAlwaysMsgf(MoveToggleButton->OnReleased.IsBound(), TEXT("OnMoveToggleButtonClicked not bound to MoveToggleButton"));
	ReverseToggleButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnReverseToggleClicked);
	ensureAlwaysMsgf(ReverseToggleButton->OnReleased.IsBound(), TEXT("OnReverseToggleClicked not bound to ReverseToggleButton"));
	AddSplineDataButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnAddSplineDataButtonClicked);
	ensureAlwaysMsgf(AddSplineDataButton->OnReleased.IsBound(), TEXT("OnAddSplineDataButtonClicked not bound to AddSplineDataButton"));
	AddTargetShapeButton->OnReleased.AddDynamic(this, &UControlPanelWidget::OnAddTargetShapeButtonClicked);
	ensureAlwaysMsgf(AddTargetShapeButton->OnReleased.IsBound(), TEXT("OnAddTargetShapeButtonClicked not bound to AddTargetShapeButton"));

	// Slider Binds
	TargetSpeedSlider->OnValueChanged.AddDynamic(this, &UControlPanelWidget::OnTargetSpeedSliderChanged);
	ensureAlwaysMsgf(TargetSpeedSlider->OnValueChanged.IsBound(), TEXT("OnTargetSpeedSliderChanged not bound to TargetSpeedSlider"));
	TargetSizeSlider->OnValueChanged.AddDynamic(this, &UControlPanelWidget::OnTargetSizeSliderChanged);
	ensureAlwaysMsgf(TargetSizeSlider->OnValueChanged.IsBound(), TEXT("OnTargetSizeSliderChanged not bound to TargetSizeSlider"));

	// Initialize UI state
		// Default Canvas
	WidgetSwitcher->SetActiveWidgetIndex(0);
		// Settings Button
	MovementSettingsButton->SetToolTipText(FText::FromString(TEXT("Movement Settings")));
	TargetShapeSettingsButton->SetToolTipText(FText::FromString(TEXT("Target Shape Settings")));
	TargetColorSettingsButton->SetToolTipText(FText::FromString(TEXT("Target Color Settings")));
	SplineCollectionSettingsButton->SetToolTipText(FText::FromString(TEXT("Path Settings")));
		// Movement
	MoveToggleButton->SetBackgroundColor(FLinearColor(0, 0.3f, 1, 1));
	ReverseToggleButton->SetBackgroundColor(FLinearColor::White);
	ReverseToggleText->SetColorAndOpacity(FLinearColor::Black);
	TargetSpeedSlider->SetMaxValue(1000.f);
		// Target
	TargetShapeScrollBox->SetOrientation(Orient_Horizontal);
	PopulateTargetShapeScrollBox();
	TargetColorScrollBox->SetOrientation(Orient_Horizontal);
	PopulateTargetColorScrollBox();
	TargetSizeSlider->SetMinValue(1.0f);
	TargetSizeSlider->SetMaxValue(100.0f);
	
		//Spline Collection
	SplineCollectionScrollBox->SetOrientation(Orient_Horizontal);
	PopulateSplineCollectionScrollBox();
	SplineCollectionScrollBox->HasAnyChildren() ? EmptyListStatusText->SetVisibility(ESlateVisibility::Collapsed) : EmptyListStatusText->SetVisibility(ESlateVisibility::Visible);

	bMoveTargetEnabled = false;
	bReverseTargetEnabled = false;
	CurrentSplineSlotIndex = 1; // Initialize slot index
	CurrentTargetShapeSlotIndex = 1; // Initialize shape slot index
}

void UControlPanelWidget::SyncSpeedUIWithTargetManager(float InInitialSpeed)
{
	if (!ensureAlways(TargetSpeedSlider) || !ensureAlways(TargetSpeedText)) return;

	// Set the slider's initial position with clamping
	const float ClampedSpeed = FMath::Clamp(InInitialSpeed, TargetSpeedSlider->GetMinValue(), TargetSpeedSlider->GetMaxValue());
	TargetSpeedSlider->SetValue(ClampedSpeed);

	// Set the initial text based on the clamped speed
	const float SpeedInMeters = ClampedSpeed / 100.f; // Convert cm/s to m/s
	const FString SpeedString = FString::Printf(TEXT("%.2f m/s"), SpeedInMeters);
	TargetSpeedText->SetText(FText::FromString(SpeedString));
}

void UControlPanelWidget::SyncTargetSizeUIWithTargetManager(float InInitialSize)
{
	if (!ensureAlways(TargetSizeSlider) || !ensureAlways(TargetSizeText)) return;

	// Set the slider's initial position with clamping
	const float ClampedSize = FMath::Clamp(InInitialSize, TargetSizeSlider->GetMinValue(), TargetSizeSlider->GetMaxValue());
	TargetSizeSlider->SetValue(ClampedSize);

	// Set the initial text based on the clamped speed
	const float SizeInCm = ClampedSize / 100.f; // Convert cm/s to m/s
	const FString SpeedString = FString::Printf(TEXT("%.2f m/s"), SizeInCm);
	TargetSpeedText->SetText(FText::FromString(SpeedString));
}

void UControlPanelWidget::SetTargetSpeedUI(float InUIValue)
{
	if (!ensureAlways(TargetSpeedSlider) || !ensureAlways(TargetSpeedText)) return;

	// Clamp the input value
	const float ClampedValue = FMath::Clamp(InUIValue, TargetSpeedSlider->GetMinValue(), TargetSpeedSlider->GetMaxValue());

	// Set Text Block
	const float SpeedInMeters = ClampedValue / 100.f; // Convert cm/s to m/s
	const FString SpeedString = FString::Printf(TEXT("%.2f m/s"), SpeedInMeters);
	TargetSpeedText->SetText(FText::FromString(SpeedString));

	// Set Slider Value
	TargetSpeedSlider->SetValue(ClampedValue);
}


void UControlPanelWidget::OnMovementSettingsButtonClicked()
{
	if (!ensureAlways(WidgetSwitcher)) return;
	WidgetSwitcher->SetActiveWidgetIndex(0);
}

void UControlPanelWidget::OnTargetShapeSettingsButtonClicked()
{
	if (!ensureAlways(WidgetSwitcher)) return;
	WidgetSwitcher->SetActiveWidgetIndex(1);
}

void UControlPanelWidget::OnTargetColorSettingsButtonClicked()
{
	if (!ensureAlways(WidgetSwitcher)) return;
	WidgetSwitcher->SetActiveWidgetIndex(2);
}

void UControlPanelWidget::OnSplineCollectionSettingsButtonClicked()
{
	if (!ensureAlways(WidgetSwitcher)) return;
	WidgetSwitcher->SetActiveWidgetIndex(3);
}

void UControlPanelWidget::ResetUIAndMovement()
{
	// Reset Movement
	if (!ensureAlways(MoveTargetDelegate.IsBound()) || !ensureAlways(ReverseTargetDelegate.IsBound())) return;

	// Set movement to "Stop" (false) and direction to "Forward" (false)
	ensureAlways(MoveTargetDelegate.Execute(false));
	ensureAlways(ReverseTargetDelegate.Execute(false));

	// Update UI state
	bMoveTargetEnabled = false;
	bReverseTargetEnabled = false;

	WidgetSwitcher->SetActiveWidgetIndex(0);

	MoveToggleButton->SetBackgroundColor(FLinearColor(0, 0.3f, 1, 1));
	MoveToggleText->SetText(FText::FromString("Start"));
	ReverseToggleButton->SetBackgroundColor(FLinearColor::White);
	ReverseToggleText->SetColorAndOpacity(FLinearColor::Black);
}


void UControlPanelWidget::OnMoveToggleButtonClicked()
{
	// Toggle the state
	bMoveTargetEnabled = !bMoveTargetEnabled;

	// Inform TargetManager of the new state
	if (!ensureAlways(MoveTargetDelegate.IsBound())) return;
	bool bDelegateSuccess = MoveTargetDelegate.Execute(bMoveTargetEnabled);
	if (!ensureAlways(bDelegateSuccess)){
		// If delegate fails, revert state
		bMoveTargetEnabled = !bMoveTargetEnabled;
		return;
	}

	// If target is now moving
	if (bMoveTargetEnabled){
		MoveToggleButton->SetBackgroundColor(FLinearColor(1, 0, 0.015f, 1));
		MoveToggleText->SetText(FText::FromString("Stop"));
	}
	// If target is now stopped
	else{
		MoveToggleButton->SetBackgroundColor(FLinearColor(0, 0.3f, 1, 1));
		MoveToggleText->SetText(FText::FromString("Start"));
	}
}

void UControlPanelWidget::OnReverseToggleClicked()
{
	// Toggle the state
	bReverseTargetEnabled = !bReverseTargetEnabled;

	if (!ensureAlways(ReverseTargetDelegate.IsBound())) return;
	bool bDelegateSuccess = ReverseTargetDelegate.Execute(bReverseTargetEnabled);
	if (!ensureAlways(bDelegateSuccess)){
		// If delegate fails, revert state
		bReverseTargetEnabled = !bReverseTargetEnabled;
		return;
	}

	// If reverse is now enabled
	if (bReverseTargetEnabled){
		ReverseToggleButton->SetBackgroundColor(FLinearColor::Black);
		ReverseToggleText->SetColorAndOpacity(FLinearColor::White);
	}
	// If reverse is now disabled (moving forward)
	else{
		ReverseToggleButton->SetBackgroundColor(FLinearColor::White);
		ReverseToggleText->SetColorAndOpacity(FLinearColor::Black);
	}
}

void UControlPanelWidget::OnTargetSpeedSliderChanged(float InValue)
{
	if (!ensureAlways(TargetSpeedSlider) || !ensureAlways(TargetSpeedText)) return;

	// Determine the correct, clamped value
	const float MinValue = TargetSpeedSlider->GetMinValue();
	const float ClampedValue = FMath::Max(InValue, MinValue);

	// Update the Text Block using the clamped value
	const float SpeedInMeters = ClampedValue / 100.f; // Convert cm/s to m/s
	const FString SpeedString = FString::Printf(TEXT("%.2f m/s"), SpeedInMeters);
	TargetSpeedText->SetText(FText::FromString(SpeedString));

	// Execute the delegate to inform the TargetManager
	if (TargetSpeedDelegate.IsBound()){
		TargetSpeedDelegate.Execute(ClampedValue);
	}
}

void UControlPanelWidget::OnAddSplineDataButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!ensureAlways(DesktopPlatform)){
		return;
	}

	TArray<FString> OutFiles;
	const FString FileTypeCSV = TEXT("CSV Files (*.csv)|*.csv");

	// Get parent window handle
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(TakeWidget());
	void* ParentWindowHandle = (ParentWindow.IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	// Open Dialog: Get the source CSV file
	bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Import Spline CSV"),
		FPaths::ProjectDir(), // Default path
		TEXT(""), // Default file
		FileTypeCSV,
		EFileDialogFlags::None, // Single file selection
		OutFiles
	);

	if (!bOpened || OutFiles.Num() <= 0) return; // User cancelled

	const FString FilePath = OutFiles[0];
	const FString FileName = FPaths::GetBaseFilename(FilePath);

	// Parse CSV
	TArray<FVector> Locations, Tangents;
	ParseSplineCSV(FilePath, Locations, Tangents);

	if (Locations.Num() <= 0) return; // Parsing failed or file was empty

	// Save Asssets (Editor vs. Runtime)
#if WITH_EDITOR
	// Get the destination path for the new asset
	const FString DefaultSavePath = FPaths::ProjectContentDir() / TEXT("Assets/SplineCollection");
	const FString DefaultSaveName = FileName + TEXT(".uasset");
	const FString FileTypeUAsset = TEXT("UAsset File (*.uasset)|*.uasset");

	TArray<FString> OutAssetFiles;
	bool bSaved = DesktopPlatform->SaveFileDialog(
		ParentWindowHandle,
		TEXT("Save New Spline Data Asset"),
		DefaultSavePath,
		DefaultSaveName,
		FileTypeUAsset,
		EFileDialogFlags::None,
		OutAssetFiles
	);

	if (!bSaved || OutAssetFiles.Num() <= 0) return; // User cancelled save

	// Convert the absolute disk path to a Package path
	FString SaveFilePath = OutAssetFiles[0];
	FString AssetName = FPaths::GetBaseFilename(SaveFilePath);
	FString AssetPath = FPaths::GetPath(SaveFilePath);
	FString ContentPath = FPaths::ProjectContentDir();

	if (!AssetPath.StartsWith(ContentPath)){
		UE_LOG(LogTemp, Warning, TEXT("Asset must be saved inside the project's Content folder."));
		return; // Abort
	}

	// Get the relative path from Content (e.g., "Assets/SplineCollection")
	FString RelativePath = AssetPath;
	FPaths::MakePathRelativeTo(RelativePath, *ContentPath);
	
	// Construct the package path (e.g., "/Game/Assets/SplineCollection/MyNewSpline")
	FString PackageName = TEXT("/Game") / RelativePath / AssetName;
	FPaths::RemoveDuplicateSlashes(PackageName);

	// Create the package and the asset object
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	
	// Check if asset already exists
	if (FindObject<UObject>(nullptr, *PackageName) != nullptr){
		ensureAlwaysMsgf(false, TEXT("Asset already exists: %s. Please choose a different name."), *PackageName);
		// TODO: Add a user-facing error popup
		return;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->MarkAsFullyLoaded();

	USplineDataAsset* NewAsset = NewObject<USplineDataAsset>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_MarkAsNative);
	ensureAlwaysMsgf(NewAsset, TEXT("Failed to create new asset in package: %s"), *PackageName);
	
	// Fill the asset with data and save the package
	NewAsset->SplineDataName = FName(*AssetName);
	NewAsset->SplinePointsLocations = Locations;
	NewAsset->SplinePointsTangents = Tangents;

	// Mark the package as dirty so it gets saved
	Package->SetDirtyFlag(true);

	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(NewAsset);

	// Save the package to disk
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = true;
	SaveArgs.bWarnOfLongFilename = true;
	SaveArgs.SaveFlags = SAVE_NoError;
	
	bool bPackageSaved = UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs);
	ensureAlwaysMsgf(bPackageSaved, TEXT("Failed to save package: %s"), *PackageName);

	// Add this new persistent asset to the UI and cache
	AddSplineDataToScrollBox(NewAsset, CurrentSplineSlotIndex);

#else
	// Create a new transient data asset. This will exist only in memory.
	USplineDataAsset* NewAsset = NewObject<USplineDataAsset>(this, FName(FileName), RF_Transient);
	NewAsset->SplineDataName = FName(FileName);
	NewAsset->SplinePointsLocations = Locations;
	NewAsset->SplinePointsTangents = Tangents;

	// Add this new asset to the UI and cache
	AddSplineDataToScrollBox(NewAsset, CurrentSplineSlotIndex);

#endif
}

void UControlPanelWidget::OnAddTargetShapeButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!ensureAlways(DesktopPlatform)){
		return;
	}

	TArray<FString> OutFiles;
	const FString FileTypeCSV = TEXT("CSV Files (*.csv)|*.csv");

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(TakeWidget());
	void* ParentWindowHandle = (ParentWindow.IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	bool bOpened = DesktopPlatform->OpenFileDialog(ParentWindowHandle,TEXT("Import Shape CSV"),FPaths::ProjectDir(),TEXT(""), FileTypeCSV,EFileDialogFlags::None,OutFiles);

	if (!bOpened || OutFiles.Num() <= 0) return; // User cancelled

	const FString FilePath = OutFiles[0];
	const FString FileName = FPaths::GetBaseFilename(FilePath);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	ParseShapeCSV(FilePath, Vertices, Triangles);

	if (Vertices.Num() <= 0 || Triangles.Num() <= 0){
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse shape CSV or file was empty: %s"), *FilePath);
		return; // Parsing failed or file was empty
	}

#if WITH_EDITOR
	// Get the destination path for the new asset
	const FString DefaultSavePath = FPaths::ProjectContentDir() / TEXT("Assets/TargetProperties/Shapes");
	const FString DefaultSaveName = FileName + TEXT(".uasset");
	const FString FileTypeUAsset = TEXT("UAsset File (*.uasset)|*.uasset");

	TArray<FString> OutAssetFiles;
	bool bSaved = DesktopPlatform->SaveFileDialog(ParentWindowHandle,TEXT("Save New Shape Data Asset"),DefaultSavePath,DefaultSaveName,FileTypeUAsset,EFileDialogFlags::None,OutAssetFiles);

	if (!bSaved || OutAssetFiles.Num() <= 0) return; // User cancelled save

	FString SaveFilePath = OutAssetFiles[0];
	FString AssetName = FPaths::GetBaseFilename(SaveFilePath);
	FString AssetPath = FPaths::GetPath(SaveFilePath);
	FString ContentPath = FPaths::ProjectContentDir();

	if (!AssetPath.StartsWith(ContentPath)){
		UE_LOG(LogTemp, Warning, TEXT("Asset must be saved inside the project's Content folder."));
		return;
	}

	FString RelativePath = AssetPath;
	FPaths::MakePathRelativeTo(RelativePath, *ContentPath);
	
	FString PackageName = TEXT("/Game") / RelativePath / AssetName;
	FPaths::RemoveDuplicateSlashes(PackageName);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	
	if (FindObject<UObject>(nullptr, *PackageName) != nullptr){
		ensureAlwaysMsgf(false, TEXT("Asset already exists: %s. Please choose a different name."), *PackageName);
		return;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->MarkAsFullyLoaded();

	UTargetShapeDataAsset* NewAsset = NewObject<UTargetShapeDataAsset>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_MarkAsNative);
	ensureAlwaysMsgf(NewAsset, TEXT("Failed to create new asset in package: %s"), *PackageName);
	
	// Fill the asset with data
	NewAsset->Vertices = Vertices;
	NewAsset->Triangles = Triangles;
	// TODO: Add normals, UVs, and convex hull to CSV parser if needed

	Package->SetDirtyFlag(true);
	FAssetRegistryModule::AssetCreated(NewAsset);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = true;
	SaveArgs.bWarnOfLongFilename = true;
	SaveArgs.SaveFlags = SAVE_NoError;
	
	bool bPackageSaved = UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs);
	ensureAlwaysMsgf(bPackageSaved, TEXT("Failed to save package: %s"), *PackageName);

	AddTargetShapeToScrollBox(NewAsset, CurrentTargetShapeSlotIndex);

#else
	// Create a new transient data asset.
	UTargetShapeDataAsset* NewAsset = NewObject<UTargetShapeDataAsset>(this, FName(FileName), RF_Transient);
	NewAsset->Vertices = Vertices;
	NewAsset->Triangles = Triangles;

	AddTargetShapeToScrollBox(NewAsset, CurrentTargetShapeSlotIndex);
#endif
}


void UControlPanelWidget::PopulateSplineCollectionScrollBox()
{
	if (!ensureAlways(SplineCollectionScrollBox) || !ensureAlways(SplineCollectionSlotClass.IsValid())) return;

	// Clear existing children and cache
	SplineCollectionScrollBox->ClearChildren();
	CachedSplineAssets.Empty();
	CurrentSplineSlotIndex = 1; // Reset index

	// Find all SplineDataAsset assets in the project
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	// Use FARFilter for consistency
	FARFilter Filter;
	Filter.ClassPaths.Add(USplineDataAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Assets/SplineCollection")); // Assuming this is the main path
	Filter.bRecursivePaths = true;
	
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	for (const FAssetData& Data : AssetData){
		USplineDataAsset* SplineAsset = Cast<USplineDataAsset>(Data.GetAsset());
		if (ensureAlways(SplineAsset)){
			// Use the helper to add the asset
			AddSplineDataToScrollBox(SplineAsset, CurrentSplineSlotIndex);
		}
	}

	// Update empty list text
	SplineCollectionScrollBox->HasAnyChildren() ? EmptyListStatusText->SetVisibility(ESlateVisibility::Collapsed) : EmptyListStatusText->SetVisibility(ESlateVisibility::Visible);
}


void UControlPanelWidget::PopulateTargetShapeScrollBox()
{
	if (!ensureAlways(TargetShapeScrollBox) || !ensureAlways(TargetShapeSlotClass.IsValid())) return;

	TargetShapeScrollBox->ClearChildren();
	CachedTargetShapes.Empty(); // Clear the shape cache
	CurrentTargetShapeSlotIndex = 1; // Reset index
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UTargetShapeDataAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Assets/TargetProperties/Shapes"));
	Filter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	for (const FAssetData& Data : AssetData)
	{
		UControlPanelListSlot* NewSlot = CreateWidget<UControlPanelListSlot>(this, TargetShapeSlotClass.Get());
		if (!ensureAlways(NewSlot)) continue;

		NewSlot->SlotType = EControlPanelSlotType::EYE_TARGET_SHAPE;
		
		const FName AssetName = Data.AssetName;
		NewSlot->SetupSlotText(CurrentTargetShapeSlotIndex++, AssetName);
		
		// Cache the Soft Object Path using the AssetName as the key
		CachedTargetShapes.Add(AssetName, Data.GetSoftObjectPath());
		
		BindToSlotSelected(NewSlot);
		TargetShapeScrollBox->AddChild(NewSlot);
	}
}

void UControlPanelWidget::PopulateTargetColorScrollBox()
{
	if (!ensureAlways(TargetColorScrollBox) || !ensureAlways(TargetColorSlotClass.IsValid())) return;

	TargetColorScrollBox->ClearChildren();
	CachedTargetColors.Empty(); // Clear the color cache
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UTargetPropertiesDataAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Assets/TargetProperties/Colors"));
	Filter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	int32 CurrentIndex = 1;
	for (const FAssetData& Data : AssetData)
	{
		UTargetPropertiesDataAsset* Asset = Cast<UTargetPropertiesDataAsset>(Data.GetAsset());
		
		// Check if it's a valid asset and it's a COLOR
		if (!ensureAlways(Asset) || Asset->TargetPropertyType != ETargetPropertyType::COLOR) continue;
		
		UControlPanelListSlot* NewSlot = CreateWidget<UControlPanelListSlot>(this, TargetColorSlotClass.Get());
		if (!ensureAlways(NewSlot)) return;
		NewSlot->SlotType = EControlPanelSlotType::EYE_TARGET_COLOR;
		
		// Use AssetName for the UI text
		NewSlot->SetupSlotText(CurrentIndex++, Asset->AssetName);
		// Set Target Color for the image
		NewSlot->SetupSlotImageColor(Asset->TargetColor);
		
		// Cache the TargetColor using AssetName as the key
		CachedTargetColors.Add(Asset->AssetName, Asset->TargetColor);
		
		BindToSlotSelected(NewSlot);
		TargetColorScrollBox->AddChild(NewSlot);
	}
}

void UControlPanelWidget::AddSplineDataToScrollBox(USplineDataAsset* InNewAsset, int32& InSlotIndex)
{
	if (!InNewAsset || !SplineCollectionSlotClass.IsValid() || !SplineCollectionScrollBox) return;

	UControlPanelListSlot* NewSlot = CreateWidget<UControlPanelListSlot>(this, SplineCollectionSlotClass.Get());
	if (!ensureAlways(NewSlot)) return;

	// Assign the SlotType
	NewSlot->SlotType = EControlPanelSlotType::SPLINE_DATA;
	
	// Use the asset's FName as a fallback if SplineDataName is empty
	FName DisplayName = InNewAsset->SplineDataName.IsNone() ? InNewAsset->GetFName() : InNewAsset->SplineDataName;

	// Ensure display name is unique if we're adding transients
	FName UniqueDisplayName = DisplayName;
	int32 Suffix = 1;
	while (CachedSplineAssets.Contains(UniqueDisplayName)){
		UniqueDisplayName = FName(*FString::Printf(TEXT("%s_%d"), *DisplayName.ToString(), Suffix++));
	}
	
	NewSlot->SetupSlotText(InSlotIndex++, UniqueDisplayName);
	SplineCollectionScrollBox->AddChild(NewSlot);

	// Bind To individual Spline Slot click 
	BindToSlotSelected(NewSlot);

	// Add to cache using the unique name
	CachedSplineAssets.Add(UniqueDisplayName, InNewAsset);

	// Update empty list text
	EmptyListStatusText->SetVisibility(ESlateVisibility::Collapsed);
}

void UControlPanelWidget::AddTargetShapeToScrollBox(UTargetShapeDataAsset* InNewAsset, int32& InSlotIndex)
{
	if (!InNewAsset || !TargetShapeSlotClass.IsValid() || !TargetShapeScrollBox) return;

	UControlPanelListSlot* NewSlot = CreateWidget<UControlPanelListSlot>(this, TargetShapeSlotClass.Get());
	if (!ensureAlways(NewSlot)) return;

	NewSlot->SlotType = EControlPanelSlotType::EYE_TARGET_SHAPE;

	// Use the asset's FName (which is guaranteed unique)
	const FName AssetName = InNewAsset->GetFName();
	
	NewSlot->SetupSlotText(InSlotIndex++, AssetName);
	
	CachedTargetShapes.Add(AssetName, FSoftObjectPath(InNewAsset));
	
	BindToSlotSelected(NewSlot);
	TargetShapeScrollBox->AddChild(NewSlot);
}

void UControlPanelWidget::ParseSplineCSV(const FString& FilePath, TArray<FVector>& OutLocations,TArray<FVector>& OutTangents)
{
	OutLocations.Empty();
	OutTangents.Empty();
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath)){
		ensureAlwaysMsgf(false, TEXT("Failed to load CSV file: %s"), *FilePath);
		return;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, true); // true to skip empty lines

	// Assumed CSV format: X,Y,Z,TangentX,TangentY,TangentZ
	// We skip the first line, assuming it's a header
	for (int32 i = 1; i < Lines.Num(); ++i){
		const FString& Line = Lines[i];
		TArray<FString> Values;
		Line.ParseIntoArray(Values, TEXT(","));

		if (Values.Num() == 6){  // Expecting 6 values
			FVector Location(FCString::Atof(*Values[0]), FCString::Atof(*Values[1]), FCString::Atof(*Values[2]));
			FVector Tangent(FCString::Atof(*Values[3]), FCString::Atof(*Values[4]), FCString::Atof(*Values[5]));
			OutLocations.Add(Location);
			OutTangents.Add(Tangent);
		}
		ensureAlwaysMsgf((Values.Num() == 6), TEXT("Skipping CSV line %d, incorrect format: %s"), i, *Line);
	}
}

void UControlPanelWidget::ParseShapeCSV(const FString& In, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles)
{
	OutVertices.Empty();
	OutTriangles.Empty();
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *In)){
		UE_LOG(LogTemp, Error, TEXT("Failed to load CSV file: %s"), *In);
		return;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, true); 

	bool bHadError = false;
	
	for (int32 i = 1; i < Lines.Num(); ++i)
	{
		const FString& Line = Lines[i];
		TArray<FString> Values;
		Line.ParseIntoArray(Values, TEXT(","));

		// 1. Check for empty lines
		if (Values.Num() <= 0) continue;

		const FString Type = Values[0].TrimStartAndEnd().ToLower();

		if (Type == TEXT("v") && Values.Num() >= 4){ // Changed == 4 to >= 4 to allow extra data (UVs etc)
			FVector Vertex(FCString::Atof(*Values[1]), FCString::Atof(*Values[2]), FCString::Atof(*Values[3]));
			OutVertices.Add(Vertex);
		}
		else if (Type == TEXT("t") && Values.Num() >= 4){ // Changed == 4 to >= 4
			OutTriangles.Add(FCString::Atoi(*Values[1]));
			OutTriangles.Add(FCString::Atoi(*Values[2]));
			OutTriangles.Add(FCString::Atoi(*Values[3]));
		}
		else{
			bHadError = true;
			continue; 
		}
		ensureAlwaysMsgf(bHadError, TEXT("Skipping Shape CSV line %d, incorrect format or type: %s"), i, *Line);
	}
}

void UControlPanelWidget::OnTargetSizeSliderChanged(float InValue)
{
	if (!ensureAlways(TargetSizeSlider) || !ensureAlways(TargetSizeText)) return;

	// Determine the correct, clamped value
	const float MinValue = TargetSizeSlider->GetMinValue();
	const float ClampedValue = FMath::Max(InValue, MinValue);

	// Update the Text Block using the clamped value
	const FString SizeString = FString::Printf(TEXT("%.1f cm"), ClampedValue);
	TargetSizeText->SetText(FText::FromString(SizeString));

	// Execute the delegate to inform the TargetManager
	if (TargetSizeDelegate.IsBound()){
		TargetSizeDelegate.Execute(ClampedValue);
	}
}

void UControlPanelWidget::SetTargetSizeUI(float InUIValue)
{
}

void UControlPanelWidget::BindToSlotSelected(UControlPanelListSlot*& InControlPanelListSlot)
{
	if (!ensureAlways(InControlPanelListSlot)) return;

	InControlPanelListSlot->OnControlPanelSlotDelegate.BindUObject(this, &UControlPanelWidget::OnSlotSelected);
	ensureAlways(InControlPanelListSlot->OnControlPanelSlotDelegate.IsBoundToObject(this));
}

void UControlPanelWidget::OnSlotSelected(EControlPanelSlotType InSlotType, const FName& InSlotName)
{
	switch (InSlotType){
	case EControlPanelSlotType::EYE_TARGET_COLOR:{
			FLinearColor* FoundColor = CachedTargetColors.Find(InSlotName);
			if (!ensureAlways(FoundColor)) return;

			TargetColorDelegate.ExecuteIfBound(*FoundColor);
			break;
		}
	case EControlPanelSlotType::EYE_TARGET_SHAPE:{
			// Find the path in the shape cache
			FSoftObjectPath* FoundPath = CachedTargetShapes.Find(InSlotName);
			if (!ensureAlways(FoundPath)){
				ensureAlwaysMsgf(false, TEXT("Selected shape '%s' not found in cache."), *InSlotName.ToString());
				return;
			}

			// Execute the delegate with the found path
			TargetShapeDelegate.ExecuteIfBound(*FoundPath);
			break;
		}
	case EControlPanelSlotType::SPLINE_DATA:{
		TObjectPtr<USplineDataAsset>* FoundAssetPtr = CachedSplineAssets.Find(InSlotName);

		if (!ensureAlways(FoundAssetPtr) || !ensureAlways(IsValid(*FoundAssetPtr))){
			ensureAlwaysMsgf(false, TEXT("Selected spline '%s' not found in cache."), *InSlotName.ToString());
			return;
		}

		// non-const pointer to pass to the delegate
		USplineDataAsset* AssetToLoad = FoundAssetPtr->Get();
		if (!ensureAlways(LoadSplineDataDelegate.IsBound())) return;
			
		const bool bDelegateSuccess = LoadSplineDataDelegate.Execute(AssetToLoad);
		if (!bDelegateSuccess) return;

		// Reset UI and movement after successful load
		ResetUIAndMovement();
		break;
	}
	case EControlPanelSlotType::UNDEFINED:
	default:
		ensureAlwaysMsgf(false, TEXT("OnSlotSelected called with UNDEFINED or unhandled SlotType!"));
		break;
	}
}