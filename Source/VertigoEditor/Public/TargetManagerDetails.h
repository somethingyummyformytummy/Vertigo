// In VertigoEditor/Private/TargetManagerDetails.h

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class ATargetManager;
class SEditableTextBox;
class FReply;

class FTargetManagerDetails : public IDetailCustomization
{
public:
	// Creates a new instance of this detail customization.
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization interface

private:
	// Handles the "Load Spline Data" button click.
	FReply OnLoadSplineDataClicked();

	// Handles the "Save Spline Data" button click.
	FReply OnSaveSplineDataClicked();

	// Handles the "Import CSV" button click.
	FReply OnImportCSVClicked();

	// Handles the "Export CSV" button click.
	FReply OnExportCSVClicked();

	// Helper function to save spline point data to a new .uasset file
	bool SaveDataToNewAsset(const TArray<FVector>& SplineLocations, const TArray<FVector>& SplineTangents,
	                        FString DefaultAssetName);

	// A weak pointer to the TargetManager actor being edited.
	TWeakObjectPtr<ATargetManager> TargetManager;

	// A shared pointer to the editable text box for the new asset name.
	TSharedPtr<SEditableTextBox> AssetNameEditableTextBox;
};
