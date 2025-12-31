// In VertigoEditor/Private/EyeTargetDetails.cpp

#include "EyeTargetDetails.h"
#include "Interaction/EyeTarget.h"
#include "Utils/TargetShapeDataAsset.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"


// For the Save/Load Dialog
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"

// To refresh the content browser
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

// For File I/O
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "EyeTargetDetails"

TSharedRef<IDetailCustomization> FEyeTargetDetails::MakeInstance()
{
	return MakeShareable(new FEyeTargetDetails);
}

void FEyeTargetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get the object being customized
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	if (SelectedObjects.Num() != 1){
		return;
	}

	SelectedEyeTarget = Cast<AEyeTarget>(SelectedObjects[0].Get());
	if (!ensureAlways(SelectedEyeTarget.IsValid())){
		return;
	}

	// Create a new category or use an existing one
	IDetailCategoryBuilder& ShapeCategory = DetailBuilder.EditCategory(FName("Shape Management"), FText::FromString("Shape Management"), ECategoryPriority::Important);

	// --- IMPORT CSV ROW ---
	FDetailWidgetRow& ImportCSVRow = ShapeCategory.AddCustomRow(LOCTEXT("ImportCSVSearch", "Import Shape CSV"));
	ImportCSVRow.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ImportCSVLabel", "Import Shape CSV"))
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
			.Text(LOCTEXT("ImportCSVButtonText", "Import CSV & Convert"))
			.Justification(ETextJustify::Center)
		]
		.OnClicked(this, &FEyeTargetDetails::OnImportCSVClicked)
		.ToolTipText(LOCTEXT("ImportCSVButtonTooltip", "Imports a CSV (Vertex, Normal, UV), creates a TargetShapeDataAsset, and applies it."))
	];

	// --- EXPORT CSV ROW ---
	FDetailWidgetRow& ExportCSVRow = ShapeCategory.AddCustomRow(LOCTEXT("ExportCSVSearch", "Export Shape CSV"));
	ExportCSVRow.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ExportCSVLabel", "Export Shape CSV"))
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
			.Text(LOCTEXT("ExportCSVButtonText", "Export Current Shape..."))
			.Justification(ETextJustify::Center)
		]
		.OnClicked(this, &FEyeTargetDetails::OnExportCSVClicked)
		.ToolTipText(LOCTEXT("ExportCSVButtonTooltip", "Exports the currently assigned TargetShapeDataAsset to a CSV file."))
	];
}

FReply FEyeTargetDetails::OnImportCSVClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFilenames;
	const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	
	const bool bFileSelected = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Shape Data from CSV"),
		DefaultPath,
		TEXT(""),
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0) return FReply::Handled();

	FString SelectedFilePath = OutFilenames[0];
	FString FileContent;
	
	if (!FFileHelper::LoadFileToString(FileContent, *SelectedFilePath)){
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *SelectedFilePath);
		return FReply::Handled();
	}

	// Containers for the new data
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	if (ParseCSVToMeshData(FileContent, Vertices, Triangles, Normals, UVs)){
		// Use the filename as the default asset name
		FString DefaultAssetName = FPaths::GetBaseFilename(SelectedFilePath);
		SaveDataToNewShapeAsset(Vertices, Triangles, Normals, UVs, DefaultAssetName);
	}

	return FReply::Handled();
}

bool FEyeTargetDetails::ParseCSVToMeshData(const FString& CSVContent, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs)
{
	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines, true);

	OutVertices.Empty();
	OutTriangles.Empty();
	OutNormals.Empty();
	OutUVs.Empty();

	int32 VertexCounter = 0;

	// Basic parser assuming "Triangle Soup" format (3 lines = 1 triangle)
	// Expected Header roughly: VertexX,VertexY,VertexZ,NormalX,NormalY,NormalZ,UVX,UVY
	for (const FString& Line : Lines)
	{
		if (Line.IsEmpty() || Line.StartsWith(TEXT("Vertex")) || Line.StartsWith(TEXT("Loc")) || Line.StartsWith(TEXT("Pos"))) {
			continue;
		}

		TArray<FString> Cells;
		Line.ParseIntoArray(Cells, TEXT(","), true);

		// We need at least 3 cells for Position. 
		// Ideally 8 for Pos(3) + Norm(3) + UV(2)
		if (Cells.Num() >= 3){
			float X = FCString::Atof(*Cells[0]);
			float Y = FCString::Atof(*Cells[1]);
			float Z = FCString::Atof(*Cells[2]);
			OutVertices.Add(FVector(X, Y, Z));

			if (Cells.Num() >= 6){
				float NX = FCString::Atof(*Cells[3]);
				float NY = FCString::Atof(*Cells[4]);
				float NZ = FCString::Atof(*Cells[5]);
				OutNormals.Add(FVector(NX, NY, NZ));
			}
			else{
				OutNormals.Add(FVector::UpVector); // Default normal
			}

			if (Cells.Num() >= 8){
				float U = FCString::Atof(*Cells[6]);
				float V = FCString::Atof(*Cells[7]);
				OutUVs.Add(FVector2D(U, V));
			}
			else{
				OutUVs.Add(FVector2D::ZeroVector);
			}

			// Add index for this vertex (Creating non-indexed geometry / Triangle Soup from CSV)
			OutTriangles.Add(VertexCounter);
			VertexCounter++;
		}
	}

	if (OutVertices.Num() == 0){
		UE_LOG(LogTemp, Warning, TEXT("No vertices parsed from CSV. Check format: X,Y,Z,nX,nY,nZ,u,v"));
		return false;
	}

	return true;
}

bool FEyeTargetDetails::SaveDataToNewShapeAsset(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs, FString DefaultAssetName)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return false;

	// Set default path to the requested specific folder
	FString DefaultPath = FPaths::ProjectContentDir() + TEXT("Assets/TargetProperties/Shapes/");
	FPaths::NormalizeDirectoryName(DefaultPath);
	
	// Ensure the directory exists (optional but good practice)
	IFileManager::Get().MakeDirectory(*DefaultPath, true);

	FString DefaultFileName = DefaultAssetName + TEXT(".uasset");

	TArray<FString> OutFilenames;
	bool bFileSelected = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Save New Target Shape Asset"),
		DefaultPath,
		DefaultFileName,
		TEXT("Unreal Asset (*.uasset)|*.uasset"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0) return false;

	FString SelectedFilePath = OutFilenames[0];
	FString PackagePath;
	if (!FPackageName::TryConvertFilenameToLongPackageName(SelectedFilePath, PackagePath)){
		UE_LOG(LogTemp, Error, TEXT("Invalid save path. Must be inside the Content folder."));
		return false;
	}

	const FString NewAssetName = FPackageName::GetShortName(PackagePath);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!ensureAlways(Package)) return false;

	UTargetShapeDataAsset* NewAsset = NewObject<UTargetShapeDataAsset>(Package, FName(*NewAssetName), RF_Public | RF_Standalone);
	if (!ensureAlways(NewAsset)) return false;

	// Fill Data
	NewAsset->Vertices = InVertices;
	NewAsset->Triangles = InTriangles;
	NewAsset->Normals = InNormals;
	NewAsset->UVs = InUVs;

	// Generate simple convex hull from vertices for collision (optional but useful)
	// For simplicity here, we just copy vertices. A real convex hull alg is complex, 
	// but Unreal's physics engine can often handle the cloud or we leave it to complex collision.
	// Leaving it empty means it uses complex collision if enabled in EyeTarget.
	NewAsset->ConvexHullVertices = InVertices; 

	FAssetRegistryModule::AssetCreated(NewAsset);
	NewAsset->MarkPackageDirty();
	Package->FullyLoad();

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	
	bool bSuccess = UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs);

	if (bSuccess){
		UE_LOG(LogTemp, Log, TEXT("Shape Data Asset saved to %s"), *PackageFileName);

		// Sync Content Browser
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(TArray<UObject*>{NewAsset});

		// Update the selected EyeTarget Actor
		if (SelectedEyeTarget.IsValid()){
			FScopedTransaction Transaction(LOCTEXT("UpdateEyeTargetShape", "Update EyeTarget Shape"));
			SelectedEyeTarget->Modify();
			
			// Update the property
			SelectedEyeTarget->DefaultShapeDataAsset = NewAsset;
			
			// Force visual update
			SelectedEyeTarget->UpdateMeshShape(NewAsset);
			
			UE_LOG(LogTemp, Log, TEXT("EyeTarget updated with new shape."));
		}
	}

	return bSuccess;
}

FReply FEyeTargetDetails::OnExportCSVClicked()
{
	if (!SelectedEyeTarget.IsValid()) return FReply::Handled();

	// Resolve the soft pointer
	UTargetShapeDataAsset* ShapeData = SelectedEyeTarget->DefaultShapeDataAsset.LoadSynchronous();

	if (!ShapeData){
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No valid Shape Data Asset is currently assigned to this Eye Target."));
		return FReply::Handled();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFilenames;
	const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const FString DefaultFile = ShapeData->GetName() + TEXT(".csv");

	bool bFileSelected = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Shape Data to CSV"),
		DefaultPath,
		DefaultFile,
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bFileSelected || OutFilenames.Num() == 0) return FReply::Handled();

	FString CSVContent = TEXT("VertexX,VertexY,VertexZ,NormalX,NormalY,NormalZ,UVX,UVY\n");

	const TArray<FVector>& Verts = ShapeData->Vertices;
	const TArray<FVector>& Norms = ShapeData->Normals;
	const TArray<FVector2D>& UVs = ShapeData->UVs;
	const TArray<int32>& Tris = ShapeData->Triangles;

	for (int32 i = 0; i < Tris.Num(); ++i)
	{
		int32 Index = Tris[i];
		if (Verts.IsValidIndex(Index)){
			FVector V = Verts[Index];
			FVector N = Norms.IsValidIndex(Index) ? Norms[Index] : FVector::UpVector;
			FVector2D UV = UVs.IsValidIndex(Index) ? UVs[Index] : FVector2D::ZeroVector;

			CSVContent += FString::Printf(TEXT("%f,%f,%f,%f,%f,%f,%f,%f\n"),
				V.X, V.Y, V.Z,
				N.X, N.Y, N.Z,
				UV.X, UV.Y
			);
		}
	}

	if (FFileHelper::SaveStringToFile(CSVContent, *OutFilenames[0])){
		UE_LOG(LogTemp, Log, TEXT("Exported Shape Data to %s"), *OutFilenames[0]);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE