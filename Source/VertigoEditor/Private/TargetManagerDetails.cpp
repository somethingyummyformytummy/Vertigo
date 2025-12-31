// In VertigoEditor/Private/TargetManagerDetails.cpp

#include "TargetManagerDetails.h"
#include "Interaction/TargetManager.h"
#include "Utils/SplineDataAsset.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Components/SplineComponent.h"
#include "PropertyHandle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "ScopedTransaction.h"
// For the Save/Load Dialog
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
// To refresh the content browser
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
// For File I/O
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"


#define LOCTEXT_NAMESPACE "TargetManagerDetails"

TSharedRef<IDetailCustomization> FTargetManagerDetails::MakeInstance()
{
	return MakeShareable(new FTargetManagerDetails);
}

void FTargetManagerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get the object being customized
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	if (SelectedObjects.Num() != 1){
		return;
	}

	TargetManager = Cast<ATargetManager>(SelectedObjects[0].Get());
	if (!TargetManager.IsValid()){
		return;
	}

	// Get the property handle and explicitly hide it
	TSharedPtr<IPropertyHandle> SplineDataAssetToLoadProperty = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(ATargetManager, SplineDataAssetToLoad));
	if (SplineDataAssetToLoadProperty.IsValid()){
		DetailBuilder.HideProperty(SplineDataAssetToLoadProperty);
	}

	// Edit the subcategory
	IDetailCategoryBuilder& SubCategory = DetailBuilder.EditCategory(FName("Target Manager"), FText::GetEmpty(),
	                                                                 ECategoryPriority::Default);

	// Add the "Load" row only if the property handle is valid
	if (ensureAlways(SplineDataAssetToLoadProperty.IsValid())){
		IDetailPropertyRow& LoadRow = SubCategory.AddProperty(SplineDataAssetToLoadProperty);
		LoadRow.CustomWidget()
		       .NameContent()
			[
				SNew(STextBlock)
				.Text(SplineDataAssetToLoadProperty->GetPropertyDisplayName())
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MinDesiredWidth(250.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SplineDataAssetToLoadProperty->CreatePropertyValueWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(2.0f))
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("LoadButtonText", "Load"))
						.Justification(ETextJustify::Center)
					]
					.OnClicked(this, &FTargetManagerDetails::OnLoadSplineDataClicked)
					.ToolTipText(LOCTEXT("LoadSplineButtonTooltip",
					                     "Loads points from the specified Spline Data Asset."))
				]
			];
	}

	// --- SAVE UASSET ROW ---
	FDetailWidgetRow& SaveRow = SubCategory.AddCustomRow(LOCTEXT("SaveAssetSearch", "Save Spline Data"));
	SaveRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NewAssetNameLabel", "Save Spline Data"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(AssetNameEditableTextBox, SEditableTextBox)
				.Text(FText::FromString("NewSplineDataAsset"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ToolTipText(
					LOCTEXT("NewAssetNameTooltip", "Enter the name for the new Spline Data Asset to be saved."))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin(2.0f))
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SaveButtonText", "Save"))
					.Justification(ETextJustify::Center)
				]
				.OnClicked(this, &FTargetManagerDetails::OnSaveSplineDataClicked)
				.ToolTipText(LOCTEXT("SaveSplineButtonTooltip",
				                     "Saves the current spline points to a new Spline Data Asset."))
			]
		];

	// --- IMPORT CSV ROW ---
	FDetailWidgetRow& ImportCSVRow = SubCategory.AddCustomRow(LOCTEXT("ImportCSVSearch", "Import CSV"));
	ImportCSVRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ImportCSVLabel", "Import CSV"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(2.0f))
			.HAlign(HAlign_Center)
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ImportCSVButtonText", "Import"))
				.Justification(ETextJustify::Center)
			]
			.OnClicked(this, &FTargetManagerDetails::OnImportCSVClicked)
			.ToolTipText(LOCTEXT("ImportCSVButtonTooltip",
			                     "Loads spline points from a .csv file and saves as a new SplineDataAsset."))
		];

	// --- EXPORT CSV ROW ---
	FDetailWidgetRow& ExportCSVRow = SubCategory.AddCustomRow(LOCTEXT("ExportCSVSearch", "Export CSV"));
	ExportCSVRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ExportCSVLabel", "Export CSV"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(2.0f))
			.HAlign(HAlign_Center)
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ExportCSVButtonText", "Export..."))
				.Justification(ETextJustify::Center)
			]
			.OnClicked(this, &FTargetManagerDetails::OnExportCSVClicked)
			.ToolTipText(LOCTEXT("ExportCSVButtonTooltip", "Saves the current spline points to a .csv file."))
		];
}

FReply FTargetManagerDetails::OnLoadSplineDataClicked()
{
	if (!ensureAlways(TargetManager.IsValid())){
		UE_LOG(LogTemp, Error, TEXT("TargetManager is invalid."));
		return FReply::Handled();
	}

	USplineComponent* SplineComp = TargetManager->TargetSplineComponent;
	USplineDataAsset* DataAsset = TargetManager->SplineDataAssetToLoad;

	if (!ensureAlways(SplineComp)){
		UE_LOG(LogTemp, Error, TEXT("TargetManager's SplineComponent is missing."));
		return FReply::Handled();
	}

	if (!ensureAlways(DataAsset)){
		UE_LOG(LogTemp, Warning, TEXT("No Spline Data Asset selected to load."));
		return FReply::Handled();
	}

	if (ensureAlways(DataAsset->SplinePointsLocations.Num() != DataAsset->SplinePointsTangents.Num())){
		UE_LOG(LogTemp, Error, TEXT("Data Asset is invalid: Location and Tangent array sizes do not match."));
		return FReply::Handled();
	}

	UE_LOG(LogTemp, Log, TEXT("Loading Spline Data from Asset: %s"), *DataAsset->GetName());

	const FScopedTransaction Transaction(LOCTEXT("LoadSplineTransaction", "Load Spline Data"));
	SplineComp->Modify();

	SplineComp->ClearSplinePoints(true);

	for (int32 i = 0; i < DataAsset->SplinePointsLocations.Num(); ++i){
		const FVector& Tangent = DataAsset->SplinePointsTangents[i];
		const FSplinePoint Point(i, DataAsset->SplinePointsLocations[i], Tangent, Tangent, FRotator::ZeroRotator,
		                         FVector(1.0f), ESplinePointType::Curve);
		SplineComp->AddPoint(Point, false);
	}

	SplineComp->UpdateSpline();

	return FReply::Handled();
}

// This function now just gathers data and calls the helper
FReply FTargetManagerDetails::OnSaveSplineDataClicked()
{
	if (!ensureAlways(TargetManager.IsValid()) || !ensureAlways(TargetManager->TargetSplineComponent)){
		UE_LOG(LogTemp, Error, TEXT("TargetManager or its SplineComponent is invalid."));
		return FReply::Handled();
	}

	USplineComponent* SplineComp = TargetManager->TargetSplineComponent;
	const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
	if (ensureAlways(NumPoints < 2)){
		UE_LOG(LogTemp, Warning, TEXT("Spline must have at least 2 points to be saved."));
		return FReply::Handled();
	}

	FString AssetName = AssetNameEditableTextBox->GetText().ToString();
	if (ensureAlways(AssetName.IsEmpty())){
		UE_LOG(LogTemp, Warning, TEXT("New Asset Name cannot be empty."));
		return FReply::Handled();
	}

	// Extract data from spline
	TArray<FVector> Locations;
	TArray<FVector> Tangents;
	Locations.Reserve(NumPoints);
	Tangents.Reserve(NumPoints);

	for (int32 i = 0; i < NumPoints; ++i){
		FVector Location, Tangent;
		SplineComp->GetLocationAndTangentAtSplinePoint(i, Location, Tangent, ESplineCoordinateSpace::Local);
		Locations.Add(Location);
		Tangents.Add(Tangent);
	}

	// Call the helper function to do the saving
	SaveDataToNewAsset(Locations, Tangents, AssetName);

	return FReply::Handled();
}

// This function now parses the CSV and calls the save helper
FReply FTargetManagerDetails::OnImportCSVClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!ensureAlways(DesktopPlatform)){
		UE_LOG(LogTemp, Warning, TEXT("Could not get DesktopPlatform module."));
		return FReply::Handled();
	}

	TArray<FString> OutFilenames;
	const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const bool bFileSelected = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Spline Data from CSV"),
		DefaultPath,
		TEXT(""), // DefaultFile
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0){
		UE_LOG(LogTemp, Log, TEXT("User canceled import dialog."));
		return FReply::Handled();
	}

	FString SelectedFilePath = OutFilenames[0];
	FString FileContent;
	if (!ensureAlways(FFileHelper::LoadFileToString(FileContent, *SelectedFilePath))){
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *SelectedFilePath);
		return FReply::Handled();
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, true);

	TArray<FVector> Locations;
	TArray<FVector> Tangents;

	for (const FString& Line : Lines){
		if (Line.IsEmpty() || Line.StartsWith(TEXT("Location.X"))) // Skip header or empty lines
		{
			continue;
		}

		TArray<FString> Cells;
		Line.ParseIntoArray(Cells, TEXT(","), true);

		if (Cells.Num() == 6){
			FVector Location(FCString::Atof(*Cells[0]), FCString::Atof(*Cells[1]), FCString::Atof(*Cells[2]));
			FVector Tangent(FCString::Atof(*Cells[3]), FCString::Atof(*Cells[4]), FCString::Atof(*Cells[5]));
			Locations.Add(Location);
			Tangents.Add(Tangent);
		}
		else{
			UE_LOG(LogTemp, Warning, TEXT("Invalid CSV line format: %s"), *Line);
		}
	}

	if (Locations.Num() == 0){
		UE_LOG(LogTemp, Warning, TEXT("No valid spline points found in CSV file."));
		return FReply::Handled();
	}

	// Data is parsed. Now call the save asset helper function.
	// We'll use the CSV's filename as the default asset name.
	FString DefaultAssetName = FPaths::GetBaseFilename(SelectedFilePath);

	UE_LOG(LogTemp, Log, TEXT("Parsed %d Spline Points from CSV. Opening save dialog..."), Locations.Num());

	SaveDataToNewAsset(Locations, Tangents, DefaultAssetName);

	return FReply::Handled();
}

// --- EXPORT CSV IMPLEMENTATION ---
FReply FTargetManagerDetails::OnExportCSVClicked()
{
	if (!ensureAlways(TargetManager.IsValid()) || !ensureAlways(TargetManager->TargetSplineComponent)){
		UE_LOG(LogTemp, Error, TEXT("TargetManager or its SplineComponent is invalid."));
		return FReply::Handled();
	}

	USplineComponent* SplineComp = TargetManager->TargetSplineComponent;
	const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
	if (NumPoints < 2){
		UE_LOG(LogTemp, Warning, TEXT("Spline must have at least 2 points to be exported."));
		return FReply::Handled();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!ensureAlways(DesktopPlatform)){
		UE_LOG(LogTemp, Warning, TEXT("Could not get DesktopPlatform module."));
		return FReply::Handled();
	}

	TArray<FString> OutFilenames;
	const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const bool bFileSelected = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Spline Data to CSV"),
		DefaultPath,
		TEXT("SplineData.csv"), // DefaultFile
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0){
		UE_LOG(LogTemp, Log, TEXT("User canceled export dialog."));
		return FReply::Handled();
	}

	FString SelectedFilePath = OutFilenames[0];
	FString CSVContent;

	// Add CSV Header
	CSVContent += TEXT("Location.X,Location.Y,Location.Z,Tangent.X,Tangent.Y,Tangent.Z\n");

	// Add Spline Point Data
	for (int32 i = 0; i < NumPoints; ++i){
		FVector Location, Tangent;
		SplineComp->GetLocationAndTangentAtSplinePoint(i, Location, Tangent, ESplineCoordinateSpace::Local);

		CSVContent += FString::Printf(TEXT("%f,%f,%f,%f,%f,%f\n"),
		                              Location.X, Location.Y, Location.Z,
		                              Tangent.X, Tangent.Y, Tangent.Z);
	}

	if (ensureAlways(FFileHelper::SaveStringToFile(CSVContent, *SelectedFilePath))){
		UE_LOG(LogTemp, Log, TEXT("Spline data exported successfully to %s"), *SelectedFilePath);
	}
	else{
		UE_LOG(LogTemp, Error, TEXT("Failed to save CSV file to %s"), *SelectedFilePath);
	}

	return FReply::Handled();
}


// This function contains the logic to create and save a new SplineDataAsset
bool FTargetManagerDetails::SaveDataToNewAsset(const TArray<FVector>& SplineLocations,
                                               const TArray<FVector>& SplineTangents, FString DefaultAssetName)
{
	if (ensureAlways(SplineLocations.Num() < 2)){
		UE_LOG(LogTemp, Warning, TEXT("Spline must have at least 2 points to be saved."));
		return false;
	}

	if (SplineLocations.Num() != SplineTangents.Num()){
		UE_LOG(LogTemp, Error, TEXT("Spline data is corrupt. Location and Tangent counts do not match."));
		return false;
	}

	if (DefaultAssetName.IsEmpty()){
		DefaultAssetName = TEXT("NewSplineDataAsset");
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!ensureAlways(DesktopPlatform)){
		UE_LOG(LogTemp, Warning, TEXT("Could not get DesktopPlatform module."));
		return false;
	}

	FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	FString DefaultFileName = DefaultAssetName + TEXT(".uasset");

	TArray<FString> OutFilenames;
	bool bFileSelected = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Save Spline Data Asset"),
		DefaultPath,
		DefaultFileName,
		TEXT("Unreal Asset (*.uasset)|*.uasset"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0){
		UE_LOG(LogTemp, Log, TEXT("User canceled save dialog."));
		return false;
	}

	FString SelectedFilePath = OutFilenames[0];
	FString PackagePath;
	if (!FPackageName::TryConvertFilenameToLongPackageName(SelectedFilePath, PackagePath)){
		UE_LOG(LogTemp, Error, TEXT("Invalid save file path selected. Asset must be saved within the Content folder."));
		return false;
	}

	const FString NewAssetName = FPackageName::GetShortName(PackagePath);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!ensureAlways(Package)){
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for path: %s"), *PackagePath);
		return false;
	}

	USplineDataAsset* NewAsset = NewObject<USplineDataAsset>(Package, FName(*NewAssetName), RF_Public | RF_Standalone);
	if (!ensureAlways(NewAsset)){
		UE_LOG(LogTemp, Error, TEXT("Failed to create new SplineDataAsset."));
		return false;
	}

	NewAsset->SplineDataName = FName(*NewAssetName); // Use the asset's own name

	// Populate the new asset with the provided data
	NewAsset->SplinePointsLocations = SplineLocations;
	NewAsset->SplinePointsTangents = SplineTangents;

	FAssetRegistryModule::AssetCreated(NewAsset);
	NewAsset->MarkPackageDirty();
	Package->FullyLoad();

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());
	bool bSuccess = UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs);

	if (ensureAlways(bSuccess)){
		UE_LOG(LogTemp, Log, TEXT("Spline data asset saved successfully to %s"), *PackageFileName);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			"AssetRegistry");
		TArray<FString> PathsToScan;
		PathsToScan.Add(FPackageName::GetLongPackagePath(PackagePath));
		AssetRegistryModule.Get().ScanPathsSynchronous(PathsToScan, true);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(
			"ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(TArray<UObject*>{NewAsset});
	}
	else{
		UE_LOG(LogTemp, Error, TEXT("Failed to save spline data asset to %s"), *PackageFileName);
	}

	return bSuccess;
}


#undef LOCTEXT_NAMESPACE
