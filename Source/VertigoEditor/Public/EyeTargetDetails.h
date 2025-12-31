// In VertigoEditor/Private/EyeTargetDetails.h

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class AEyeTarget;
class UTargetShapeDataAsset;
class FReply;

class FEyeTargetDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnImportCSVClicked();
	FReply OnExportCSVClicked();

	bool ParseCSVToMeshData(const FString& CSVContent, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs);
	bool SaveDataToNewShapeAsset(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs, FString DefaultAssetName);

	TWeakObjectPtr<AEyeTarget> SelectedEyeTarget;
};