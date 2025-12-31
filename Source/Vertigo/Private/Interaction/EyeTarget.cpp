// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/EyeTarget.h"

#include "ProceduralMeshComponent.h"
#include "Utils/TargetShapeDataAsset.h"
#include "Utils/GenericUtilFunctions.h"
#include "Engine/StreamableManager.h" 
#include "Algo/Reverse.h" 

// Sets default values
AEyeTarget::AEyeTarget()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Turn tick off b/c TargetManager will move it
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Turn tick off b/c TargetManager will move it
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));
	SetRootComponent(SceneComponent);
	
	/*MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
	MeshComponent->SetupAttachment(SceneComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TempMesh(TEXT("/Script/Engine.StaticMesh'/Engine/VREditor/BasicMeshes/SM_Cube_01.SM_Cube_01'"));
	if(TempMesh.Succeeded()){
		MeshComponent->SetStaticMesh(TempMesh.Object);
	}
	MeshComponent->SetRelativeScale3D(FVector(MeshSize));
	MeshComponent->SetCollisionProfileName(FName(TEXT("TargetProfile")));*/

	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh Component"));
	ProceduralMeshComponent->SetupAttachment(SceneComponent);
	ProceduralMeshComponent->bUseComplexAsSimpleCollision = false;
	//Improves performance for physics cooking
	//ProceduralMeshComponent->bUseAsyncCooking = true;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> TempMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Materials/Mat_Target_Inst.Mat_Target_Inst'"));
	if (TempMaterial.Succeeded()){
		BaseMaterial = TempMaterial.Object;
	}
	
	SetActorEnableCollision(true);
	
	// Set the default shape asset path
	DefaultShapeDataAsset = TSoftObjectPtr<UTargetShapeDataAsset>(FSoftObjectPath(TEXT("/Game/Assets/TargetProperties/Shapes/Octahedron.Octahedron")));
}

// Called when the game starts or when spawned
void AEyeTarget::BeginPlay()
{
	Super::BeginPlay();

	UGenericUtilFunctions::RequestSoftObjectAsyncLoad(DefaultShapeDataAsset,DefaultShapeLoadHandle,this, &AEyeTarget::OnDefaultShapeLoaded);
}

void AEyeTarget::OnDefaultShapeLoaded()
{
	// Shape
	UTargetShapeDataAsset* DefaultShapeAsset = DefaultShapeDataAsset.Get();
	ensureAlwaysMsgf(DefaultShapeAsset, TEXT("Failed to load DefaultShapeDataAsset at %s. EyeTarget will have no mesh."), *DefaultShapeDataAsset.ToString());
	if (!DefaultShapeAsset) return;
        
	UpdateMeshShape(DefaultShapeAsset);
    
	// Clear the handle
	if (DefaultShapeLoadHandle.IsValid()){
		DefaultShapeLoadHandle.Reset();
	}
    
	DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (DynamicMaterial){
		DynamicMaterial->SetVectorParameterValue(FName("Color"), CurrentColor);
		AddMaterialToProceduralMesh(DynamicMaterial);
	}
}

void AEyeTarget::UpdateMeshShape(UTargetShapeDataAsset* InShapeDataAsset)
{
	if (!ensureAlways(InShapeDataAsset)){
		return;
	}
	
	// COPY the data so we can modify it
	TArray<int32> ProcessedTriangles = InShapeDataAsset->Triangles;
	TArray<FVector> ProcessedNormals = InShapeDataAsset->Normals;

	// Check for inversion
	// For your Octahedron, Volume is +1.0, which means it needs inversion.
	const bool bIsAutoInverted = IsWindingInverted(InShapeDataAsset->Vertices, ProcessedTriangles);
	
	// Apply logic if Auto-Detected OR Manually Forced
	bool bShouldReverse = bIsAutoInverted;
	
	if (bForceReverseWinding){
		// Toggle the state if force reverse is active
		bShouldReverse = !bShouldReverse;
	}

	if (bShouldReverse){
		// UE_LOG(LogTemp, Warning, TEXT("[EyeTarget] Reversing Mesh Winding and Normals for %s"), *InShapeDataAsset->GetName());

		// 1. Reverse the Triangle Winding (fixes Geometry facing)
		Algo::Reverse(ProcessedTriangles);

		// 2. Reverse the Explicit Normals (fixes Lighting)
		if (ProcessedNormals.Num() <= 0) return;
		
		for (FVector& Normal : ProcessedNormals){
			Normal *= -1.0f;
		}
	}

	CreateProceduralMeshFromData(InShapeDataAsset->Vertices, ProcessedTriangles, ProcessedNormals, InShapeDataAsset->UVs, InShapeDataAsset->ConvexHullVertices);
    
	OnChangeMeshSize(MeshSize);
}

bool AEyeTarget::IsWindingInverted(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles)
{
	if (InVertices.Num() < 3 || InTriangles.Num() < 3) return false;

	// Signed Volume Method
	FVector Centroid = FVector::ZeroVector;
	for (const FVector& Vert : InVertices){
		Centroid += Vert;
	}
	Centroid /= InVertices.Num();

	double TotalSignedVolume = 0.0;
	const int32 NumTriangles = InTriangles.Num() / 3;

	for (int32 i = 0; i < NumTriangles; ++i){
		const int32 Index0 = InTriangles[i * 3];
		const int32 Index1 = InTriangles[i * 3 + 1];
		const int32 Index2 = InTriangles[i * 3 + 2];

		if (!InVertices.IsValidIndex(Index0) || !InVertices.IsValidIndex(Index1) || !InVertices.IsValidIndex(Index2)) continue;

		// Vectors from Centroid to Vertices
		const FVector P1 = InVertices[Index0] - Centroid;
		const FVector P2 = InVertices[Index1] - Centroid;
		const FVector P3 = InVertices[Index2] - Centroid;

		// Scalar Triple Product
		const double TetrahedronVolume = FVector::DotProduct(P1, FVector::CrossProduct(P2, P3));
		
		TotalSignedVolume += TetrahedronVolume;
	}

	// In Unreal (Left Handed System), standard CCW (Visible) triangles results in Negative Signed Volume
	// Therefore, if Volume is POSITIVE (> 0), we treat it as Inverted.
	const bool bIsInverted = TotalSignedVolume > KINDA_SMALL_NUMBER;
	//UE_LOG(LogTemp, Log, TEXT("[EyeTarget] Volume: %f | Verdict: %s"), TotalSignedVolume, bIsInverted ? TEXT("INVERTED (Will Flip)") : TEXT("NORMAL"));

	return bIsInverted;
}

void AEyeTarget::CreateProceduralMeshFromData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs, const TArray<FVector>& InConvexHull)
{
	ProceduralMeshComponent->SetActive(bRenderProceduralMesh);
	if (!bRenderProceduralMesh){
		ProceduralMeshComponent->ClearAllMeshSections();
		return;
	}

	// Create a new mesh section, replacing section 0 if it exists.
	ProceduralMeshComponent->CreateMeshSection(0, InVertices, InTriangles, InNormals, InUVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true); // bCreateCollision = true
	ProceduralMeshComponent->ClearCollisionConvexMeshes();
	
	if (InConvexHull.Num() > 0){
		// Use provided simple convex collision
		ProceduralMeshComponent->AddCollisionConvexMesh(InConvexHull);
		ProceduralMeshComponent->bUseComplexAsSimpleCollision = false;
	}
	else{
		// Fallback to using complex (per-poly) collision if no convex hull is provided
		ProceduralMeshComponent->bUseComplexAsSimpleCollision = true;
	}
}

void AEyeTarget::OnChangeMeshSize(float InNewSize)
{
	if(!ensureAlways(ProceduralMeshComponent)) return;

	MeshSize = InNewSize;
	ProceduralMeshComponent->SetRelativeScale3D(FVector(MeshSize));
}

void AEyeTarget::AddMaterialToProceduralMesh(UMaterialInstanceDynamic* InDynamicMaterial)
{
	if(!ensureAlways(InDynamicMaterial) || !ensureAlways(ProceduralMeshComponent)) return;
	ProceduralMeshComponent->SetMaterial(0, InDynamicMaterial);
}

void AEyeTarget::ChangeMaterialColor(const FLinearColor& InNewColor)
{
	CurrentColor = InNewColor;
	
	if(!DynamicMaterial) return;
	DynamicMaterial->SetVectorParameterValue(FName("Color"), InNewColor);
}


void AEyeTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Cancel any pending async loads
	if (DefaultShapeLoadHandle.IsValid()){
		DefaultShapeLoadHandle->CancelHandle();
		DefaultShapeLoadHandle.Reset();
	}
}
