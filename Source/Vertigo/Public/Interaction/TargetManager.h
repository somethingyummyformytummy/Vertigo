// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetManager.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class AEyeTarget;
class USplineComponent;
class USplineDataAsset;
class UTargetShapeDataAsset;
class UControlPanelWidget;
class AVertigoGameMode;
struct FStreamableHandle;

UCLASS(HideCategories=(Rendering, Replication, Collision, HLOD, Physics, Networking, Input, Actor, Cooking, Level_Instance, World_Partition, Data_Layers))
class VERTIGO_API ATargetManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATargetManager();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called when properties are changed in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneComponent;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USplineComponent> TargetSplineComponent;

	// Spline Visualization
	UPROPERTY(VisibleAnywhere, Category = "Spline | Visualization")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SplinePointsHISMComponent;

	// Path
	UPROPERTY(VisibleAnywhere, Category = "Spline | Visualization")
	TObjectPtr<UProceduralMeshComponent> SplinePathProceduralMesh;

	UPROPERTY(EditAnywhere, Category = "Spline | Visualization")
	TObjectPtr<UStaticMesh> SplinePointStaticMesh;

	UPROPERTY(EditAnywhere, Category = "Spline | Visualization")
	float SplinePointSize = 0.05f;

	// Path Diameter
	UPROPERTY(EditAnywhere, Category = "Spline | Visualization", meta = (ClampMin = "0.1"))
	float PathWidth = 0.8f;

	// Distance between length-wise segments. Lower = smoother curves
	UPROPERTY(EditAnywhere, Category = "Spline | Visualization", meta = (ClampMin = "5.0"))
	float PathResolution = 5.0f;

	// Number of sides for the tube. 4-6 is optimized for thin wires. 8-12 for thick pipes
	UPROPERTY(EditAnywhere, Category = "Spline | Visualization", meta = (ClampMin = "3", ClampMax = "32"))
	int32 PathRadialSegments = 3;

	UPROPERTY(VisibleAnywhere, Category = "Spline | Visualization")
	TObjectPtr<UMaterialInterface> BasePathMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PathMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SplinePointsMaterialInstance;

	UPROPERTY(VisibleAnywhere, Category = "Targets | Eye Target")
	TWeakObjectPtr<AEyeTarget> EyeTargetWeak;
	UPROPERTY(EditAnywhere, Category = "Targets | Eye Target")
	FLinearColor EyeTargetColor;
	UPROPERTY(EditAnywhere, Category = "Targets | Eye Target")
	float EyeTargetSize;
	UPROPERTY(EditAnywhere, Category = "Targets | Eye Target")
	float TargetMovementSpeed = 200.f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, meta=(DisplayName="Load Spline Data"))
	TObjectPtr<USplineDataAsset> SplineDataAssetToLoad;
#endif
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AVertigoGameMode> VertigoGameMode;
	
	void OnVerifySingleton();
	void Initialize();
	bool bMarooned = false;			// if dev left a TargetManger actor in the level
	
	// Control Panel
	UPROPERTY(VisibleAnywhere)
	TSoftClassPtr<UControlPanelWidget> ControlPanelWidgetClass;
	TSharedPtr<FStreamableHandle> ControlPanelLoadHandle;
	TSharedPtr<SWindow> ControlPanelSlateWindow;
	void OnControlPanelAsyncLoaded();

// Movement
	bool OnMoveTargetEnabled(bool bMove);
	bool OnReverseTargetEnabled(bool bReverse);
	void OnTargetSpeedChanged(float InNewSpeed);
	void BindToControlPanelSplineDataLoad(UControlPanelWidget*& InControlPanelWidget);
// Target
	void OnTargetShapeChanged(const FSoftObjectPath& InShapePath);
	void OnTargetSizeChanged(float InNewSize);
// Color
	void OnTargetColorChanged(const FLinearColor& InNewColor);
	// TODO:Outline Color

	// Spline
	bool OnLoadSplineData(USplineDataAsset*& InSplineDataAsset);
	void UpdateSplineVisualization();
	void UpdateSplineColors(const FLinearColor& InBaseColor);
	void GeneratePathMesh();

	UPROPERTY(VisibleAnywhere, Category = "Targets | Eye Target")
	TSoftClassPtr<AEyeTarget> EyeTargetClass;
	TSharedPtr<FStreamableHandle> EyeTargetLoadHandle;
	void OnEyeTargetAsyncLoaded();

	TSharedPtr<FStreamableHandle> TargetShapeLoadHandle;
	void OnTargetShapeAssetLoaded();
	TSoftObjectPtr<UTargetShapeDataAsset> CurrentShapeAssetPtr;

	void MoveTargetAlongSpline(class AEyeTarget* InTargetActor, const float InDeltaTime);
	bool bMovementActive = false;
	bool bMovingForward = true;
	float SplineLength;
	float CurrentDistanceAlongSpline;
};