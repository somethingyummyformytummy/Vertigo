// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EyeTarget.generated.h"

class UProceduralMeshComponent;
class UTargetShapeDataAsset;
struct FStreamableHandle;

UCLASS() 
class VERTIGO_API AEyeTarget : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEyeTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneComponent;
	/*UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;*/
	UPROPERTY(EditAnywhere)
	TObjectPtr<UProceduralMeshComponent> ProceduralMeshComponent;

	void OnChangeMeshSize(float InNewSize);
	
	UPROPERTY(VisibleAnywhere, Category="Materials")
	TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY(VisibleAnywhere, Category="Materials")
	FLinearColor CurrentColor = FLinearColor::Red;
	
	void ChangeMaterialColor(const FLinearColor& InNewColor);

	void UpdateMeshShape(UTargetShapeDataAsset* InShapeDataAsset);

	UPROPERTY(EditAnywhere, Category="Mesh")
	bool bRenderProceduralMesh = true;

	// Failsafe: If the auto-detection gets it wrong, check this to manually flip it.
	UPROPERTY(EditAnywhere, Category="Mesh | Debug")
	bool bForceReverseWinding = false;

	UPROPERTY(EditDefaultsOnly, Category="Mesh")
	TSoftObjectPtr<UTargetShapeDataAsset> DefaultShapeDataAsset;
	
private:

	void CreateProceduralMeshFromData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs, const TArray<FVector>& InConvexHull);
	
	bool IsWindingInverted(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles);

	void AddMaterialToProceduralMesh(UMaterialInstanceDynamic* InDynamicMaterial);
	UPROPERTY(VisibleAnywhere, Transient, Category="Materials")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	
	void OnDefaultShapeLoaded();
	TSharedPtr<FStreamableHandle> DefaultShapeLoadHandle;

		
	UPROPERTY(VisibleAnywhere, Category="Mesh")
	float MeshSize = 10.f;
	// Initialize to an invalid value to force first update
	float PreviousMeshSize = -1.f;
};