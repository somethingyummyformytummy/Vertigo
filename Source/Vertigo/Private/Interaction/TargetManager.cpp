// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/TargetManager.h"
#include "Components/SplineComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Interaction/EyeTarget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "Session/VertigoGameMode.h"
#include "UI/ControlPanelWidget.h"
#include "Utils/GenericUtilFunctions.h"
#include "Styling/AppStyle.h"
#include "Utils/SplineDataAsset.h"
#include "Utils/TargetShapeDataAsset.h"

// Sets default values
ATargetManager::ATargetManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Controls movement of Target -> needs pre-physics; else delays other actors b/c this will be the prerequisite tick actor
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	SceneComponent = CreateDefaultSubobject<USceneComponent>("Scene Component");
	SetRootComponent(SceneComponent);

	TargetSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline Component"));
	TargetSplineComponent->SetupAttachment(SceneComponent);
	// TODO
	//TargetSplineComponent->SetClosedLoop();

	// Spline Visualization
	SplinePointsHISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Spline Points HISM"));
	SplinePointsHISMComponent->SetupAttachment(SceneComponent);
	SplinePointsHISMComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplinePointsHISMComponent->SetCastShadow(false);

	SplinePathProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Spline Path Proc Mesh"));
	SplinePathProceduralMesh->SetupAttachment(SceneComponent);
	SplinePathProceduralMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplinePathProceduralMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMat(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Materials/Mat_Target_Inst.Mat_Target_Inst'"));
	if (DefaultMat.Succeeded()){
		BasePathMaterial = DefaultMat.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultSphereMesh.Succeeded()){
		SplinePointStaticMesh = DefaultSphereMesh.Object;
		SplinePointStaticMesh->SetMaterial(0, BasePathMaterial);
	}

	//Added _C b/c Blueprint Class
	EyeTargetClass = TSoftClassPtr<AEyeTarget>(FSoftObjectPath(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/Interaction/BP_EyeTarget.BP_EyeTarget_C'")));
	// Load Control Panel
	ControlPanelWidgetClass = TSoftClassPtr<UControlPanelWidget>(FSoftObjectPath(TEXT("/Script/Engine.WidgetBlueprint'/Game/Blueprints/UI/WBP_ControlPanel.WBP_ControlPanel_C'")));

	SetActorEnableCollision(false);
}

// Called when the game starts or when spawned
void ATargetManager::BeginPlay()
{
	Super::BeginPlay();

	// Prune TargetManagers that might be left in the Level Editor / in the client PC (if multiplayer)
	VertigoGameMode = GetWorld()->GetAuthGameMode<AVertigoGameMode>();
	if (VertigoGameMode){
		VertigoGameMode->PruneTargetManagerDelegate.AddUObject(this, &ATargetManager::OnVerifySingleton);
		ensureAlways(VertigoGameMode->PruneTargetManagerDelegate.IsBoundToObject(this));
	}
}

void ATargetManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Only run in Editor to visualize changes immediately.
	if (GetWorld() && GetWorld()->IsGameWorld()){
		return;
	}

	UpdateSplineVisualization();
}

void ATargetManager::OnVerifySingleton()
{
	// if on the client PC or not spawned by the GameMode
	if (!HasAuthority() || VertigoGameMode->TargetManager.Get() != this){
		bMarooned = true;
		Destroy();
		return;
	}

	// Ensured that this is the only TargetManager in the session
	Initialize();
}

void ATargetManager::Initialize()
{	
	UGenericUtilFunctions::RequestSoftAsyncLoad(EyeTargetClass, EyeTargetLoadHandle, this, &ATargetManager::OnEyeTargetAsyncLoaded);
	UGenericUtilFunctions::RequestSoftAsyncLoad(ControlPanelWidgetClass, ControlPanelLoadHandle, this, &ATargetManager::OnControlPanelAsyncLoaded);

	SplineLength = TargetSplineComponent->GetSplineLength();
	UpdateSplineVisualization();
}

void ATargetManager::OnEyeTargetAsyncLoaded()
{
	// Get the loaded class
	TSubclassOf<AEyeTarget> LoadedEyeTargetClass = EyeTargetClass.Get();
	
	UWorld* World = GetWorld();
	if (!ensureAlways(World) || !ensureAlways(EyeTargetClass)) return;

	// Get the starting transform (position and rotation) from the spline's beginning
	const FVector StartLocation = TargetSplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
	const FRotator StartRotation = TargetSplineComponent->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
	const FTransform StartTransform = FTransform(StartRotation, StartLocation);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	EyeTargetWeak = World->SpawnActor<AEyeTarget>(LoadedEyeTargetClass, StartTransform, SpawnParameters);
	if (!ensureAlways(EyeTargetWeak.IsValid())) return;
	
	// Currently, eye targets have tick disabled, but if re-enabled, then manager has to run tick first
	EyeTargetWeak->AddTickPrerequisiteActor(this);
	EyeTargetWeak.Get()->ChangeMaterialColor(EyeTargetColor);

	// Sync Colors (Applies EyeTarget color + Tinted Spline color)
	UpdateSplineColors(EyeTargetColor);

	// Explicitly set the starting distance
	CurrentDistanceAlongSpline = 0.0f;

	bMovementActive = false;
	bMovingForward = true;

	// Clear the handle
	if (EyeTargetLoadHandle.IsValid()){
		EyeTargetLoadHandle.Reset();
	}
}


void ATargetManager::OnControlPanelAsyncLoaded()
{
	TSubclassOf<UControlPanelWidget> LoadedWidgetClass = ControlPanelWidgetClass.Get();
	
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!ensureAlways(PlayerController) || !ensureAlways(ControlPanelWidgetClass)) return;
		
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputModeData);
	PlayerController->bShowMouseCursor = true;

	UControlPanelWidget* ControlPanelInstance = CreateWidget<UControlPanelWidget>(PlayerController, LoadedWidgetClass);
	if (!ensureAlways(ControlPanelInstance)) return;

	const FVector2D CustomWidgetSize = ControlPanelInstance->DesignTimeSize;

	TSharedRef<SWidget> UMGWidgetContent = ControlPanelInstance->TakeWidget();

	// Bind movement delegates
	ControlPanelInstance->MoveTargetDelegate.BindUObject(this, &ATargetManager::OnMoveTargetEnabled);
	ControlPanelInstance->ReverseTargetDelegate.BindUObject(this, &ATargetManager::OnReverseTargetEnabled);
	ControlPanelInstance->TargetSpeedDelegate.BindUObject(this, &ATargetManager::OnTargetSpeedChanged);
	// Bind target property delegates
	ControlPanelInstance->TargetShapeDelegate.BindUObject(this, &ATargetManager::OnTargetShapeChanged);
	ControlPanelInstance->TargetSizeDelegate.BindUObject(this, &ATargetManager::OnTargetSizeChanged);
	ControlPanelInstance->TargetColorDelegate.BindUObject(this, &ATargetManager::OnTargetColorChanged);

	//Initialize Target Speed UI
	ControlPanelInstance->SyncSpeedUIWithTargetManager(TargetMovementSpeed);
	
	// Define the content for the new window
	TSharedRef<SWidget> WindowContent = SNew(SBorder)
		.BorderBackgroundColor(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.8f)))
		[
			UMGWidgetContent
		];

	TSharedRef<SWindow> ControlPanelWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Control Panel")))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(CustomWidgetSize)
		.SaneWindowPlacement(true)
		.IsTopmostWindow(true)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.FocusWhenFirstShown(true)
		.HasCloseButton(false)
		[
			WindowContent
		];
	
	// Add the new window to the application
	FSlateApplication::Get().AddWindow(ControlPanelWindow);
	// Force bring to front
	ControlPanelWindow->BringToFront(true);

	//Cache Control panel
	ControlPanelSlateWindow = ControlPanelWindow;

	// Bind TargetManager to Control Panel's Spline Data Load
	BindToControlPanelSplineDataLoad(ControlPanelInstance);
	
	// Clear the handle
	if (ControlPanelLoadHandle.IsValid()){
		ControlPanelLoadHandle.Reset();
	}
}

void ATargetManager::BindToControlPanelSplineDataLoad(UControlPanelWidget*& InControlPanelWidget)
{
	if (!ensureAlways(InControlPanelWidget)) return;
	InControlPanelWidget->LoadSplineDataDelegate.BindUObject(this, &ATargetManager::OnLoadSplineData);
	ensureAlways(InControlPanelWidget->LoadSplineDataDelegate.IsBoundToObject(this));
}

bool ATargetManager::OnLoadSplineData(USplineDataAsset*& InSplineDataAsset)
{
	if (!ensureAlways(InSplineDataAsset) || !ensureAlways(TargetSplineComponent)){
		return false;
	}

	// Clear existing spline points
	TargetSplineComponent->ClearSplinePoints(true); // true to update spline

	// Add new points from the data asset
	const int32 NumPoints = InSplineDataAsset->SplinePointsLocations.Num();
	if (NumPoints == 0){
		TargetSplineComponent->UpdateSpline(); // Update to clear spline
		SplineLength = 0.0f;
		CurrentDistanceAlongSpline = 0.0f;
		UpdateSplineVisualization();
		return true;
	}

	for (int32 i = 0; i < NumPoints; ++i){
		const FVector& Location = InSplineDataAsset->SplinePointsLocations[i];
		// Add point without updating spline yet for performance
		TargetSplineComponent->AddSplinePoint(Location, ESplineCoordinateSpace::Local, false);
	}

	// Set tangents (if they exist)
	const int32 NumTangents = InSplineDataAsset->SplinePointsTangents.Num();
	if (NumTangents == NumPoints){
		for (int32 i = 0; i < NumPoints; ++i){
			const FVector& Tangent = InSplineDataAsset->SplinePointsTangents[i];
			// Set both leave and arrive tangents. Adjust logic if your asset stores them differently
			TargetSplineComponent->SetTangentsAtSplinePoint(i, Tangent, Tangent, ESplineCoordinateSpace::Local, false); // Don't update spline yet
		}
	}
	ensureAlwaysMsgf(NumTangents > 0, TEXT("SplineDataAsset %s has mismatched tangents (%d) and points (%d). Using auto-tangents."), *InSplineDataAsset->GetName(), NumTangents, NumPoints);

	// Update the spline once all points/tangents are added
	TargetSplineComponent->UpdateSpline();

	// Recalculate spline length and reset target position
	SplineLength = TargetSplineComponent->GetSplineLength();
	CurrentDistanceAlongSpline = 0.0f;
	bMovingForward = true; // Reset direction

	// Move actor to start of new spline
	if(EyeTargetWeak.IsValid()){
		const FVector StartLocation = TargetSplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
		// Move without rotating
		EyeTargetWeak->SetActorLocation(StartLocation);
	}

	UpdateSplineVisualization();

	return true;
}

void ATargetManager::UpdateSplineVisualization()
{
	if (!SplinePointsHISMComponent || !SplinePathProceduralMesh || !TargetSplineComponent) return;

	//  Update Spline Points
	if (SplinePointStaticMesh && SplinePointsHISMComponent->GetStaticMesh() != SplinePointStaticMesh){
		SplinePointsHISMComponent->SetStaticMesh(SplinePointStaticMesh);
	}
	SplinePointsHISMComponent->ClearInstances();

	if (SplinePointStaticMesh){
		const int32 NumPoints = TargetSplineComponent->GetNumberOfSplinePoints();
		for (int32 i = 0; i < NumPoints; ++i){
			FTransform PointTransform = TargetSplineComponent->GetTransformAtSplinePoint(
				i, ESplineCoordinateSpace::Local);
			// Apply the user-defined Scale (so spheres aren't huge)
			PointTransform.SetScale3D(FVector(SplinePointSize));
			SplinePointsHISMComponent->AddInstance(PointTransform);
		}
	}

	// Ensure Material is correct for the points (HISM)
	if (BasePathMaterial && !SplinePointsMaterialInstance){
		SplinePointsMaterialInstance = UMaterialInstanceDynamic::Create(BasePathMaterial, this);
		SplinePointsHISMComponent->SetMaterial(0, SplinePointsMaterialInstance);
	}
	else if (SplinePointsMaterialInstance){
		SplinePointsHISMComponent->SetMaterial(0, SplinePointsMaterialInstance);
	}

	// Update Path
	GeneratePathMesh();

	// Re-apply colors in case this was a full rebuild
	UpdateSplineColors(EyeTargetColor);
}

void ATargetManager::GeneratePathMesh()
{
	if (!SplinePathProceduralMesh || !TargetSplineComponent) return;

	SplinePathProceduralMesh->ClearAllMeshSections();

	const float TotalLength = TargetSplineComponent->GetSplineLength();
	if (TotalLength <= 0.1f) return;

	// --- 1. PRE-CALCULATE SAMPLE DISTANCES ---
	// To ensure the line connects PERFECTLY with the dots, we must render a ring 
	// exactly at the distance of every Spline Point.

	TArray<float> SampleDistances;
	const int32 NumSplinePoints = TargetSplineComponent->GetNumberOfSplinePoints();
	const float SafeStepSize = FMath::Max(PathResolution, 5.0f);

	for (int32 i = 0; i < NumSplinePoints - 1; ++i){
		const float DistA = TargetSplineComponent->GetDistanceAlongSplineAtSplinePoint(i);
		const float DistB = TargetSplineComponent->GetDistanceAlongSplineAtSplinePoint(i + 1);

		// Always add the precise location of the Spline Point (The Dot)
		SampleDistances.Add(DistA);

		// Add intermediate steps between this dot and the next
		float CurrentDist = DistA + SafeStepSize;
		while (CurrentDist < DistB - KINDA_SMALL_NUMBER){
			SampleDistances.Add(CurrentDist);
			CurrentDist += SafeStepSize;
		}
	}
	// Add the final point distance
	if (NumSplinePoints > 0){
		SampleDistances.Add(TargetSplineComponent->GetDistanceAlongSplineAtSplinePoint(NumSplinePoints - 1));
	}

	// --- 2. GENERATE MESH ---

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const int32 NumSamples = SampleDistances.Num();
	const int32 RadialSegments = FMath::Clamp(PathRadialSegments, 3, 32);
	const float Radius = PathWidth * 0.5f;

	// Pre-allocate
	Vertices.Reserve(NumSamples * RadialSegments);
	Triangles.Reserve(NumSamples * RadialSegments * 6);

	for (int32 StepIndex = 0; StepIndex < NumSamples; ++StepIndex){
		const float Dist = SampleDistances[StepIndex];

		// Spline Coordinate Frame
		const FVector CenterPos = TargetSplineComponent->GetLocationAtDistanceAlongSpline(
			Dist, ESplineCoordinateSpace::Local);
		const FVector Forward = TargetSplineComponent->GetTangentAtDistanceAlongSpline(
			Dist, ESplineCoordinateSpace::Local).GetSafeNormal();
		const FVector Right = TargetSplineComponent->GetRightVectorAtDistanceAlongSpline(
			Dist, ESplineCoordinateSpace::Local).GetSafeNormal();
		const FVector Up = TargetSplineComponent->GetUpVectorAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local).
		                                          GetSafeNormal();

		// Generate Ring
		for (int32 i = 0; i < RadialSegments; ++i){
			const float Theta = 2.0f * PI * ((float)i / (float)RadialSegments);
			const float X = FMath::Cos(Theta);
			const float Y = FMath::Sin(Theta);

			const FVector Offset = (Right * X * Radius) + (Up * Y * Radius);
			const FVector VertPos = CenterPos + Offset;
			const FVector Normal = Offset.GetSafeNormal();

			Vertices.Add(VertPos);
			Normals.Add(Normal);
			Tangents.Add(FProcMeshTangent(Forward, false));

			UVs.Add(FVector2D(Dist / 100.0f, (float)i / (float)RadialSegments));
			// Default to White (Vertex Colors), color is controlled by Material Param
			Colors.Add(FLinearColor::White.ToFColor(true));

			// Triangulate
			if (StepIndex > 0){
				const int32 CurrentRingStart = StepIndex * RadialSegments;
				const int32 PrevRingStart = (StepIndex - 1) * RadialSegments;
				const int32 NextI = (i + 1) % RadialSegments;

				const int32 BottomLeft = PrevRingStart + i;
				const int32 BottomRight = PrevRingStart + NextI;
				const int32 TopLeft = CurrentRingStart + i;
				const int32 TopRight = CurrentRingStart + NextI;

				// Counter-Clockwise (Standard) Winding
				// Triangle 1
				Triangles.Add(BottomLeft);
				Triangles.Add(TopLeft);
				Triangles.Add(TopRight);

				// Triangle 2
				Triangles.Add(BottomLeft);
				Triangles.Add(TopRight);
				Triangles.Add(BottomRight);
			}
		}
	}

	SplinePathProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	if (!PathMaterialInstance && BasePathMaterial){
		PathMaterialInstance = UMaterialInstanceDynamic::Create(BasePathMaterial, this);
	}

	if (PathMaterialInstance){
		// Note: The color is now set by UpdateSplineColors, which is called after this
		SplinePathProceduralMesh->SetMaterial(0, PathMaterialInstance);
	}
}

// Called every frame
void ATargetManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bMovementActive){
		MoveTargetAlongSpline(EyeTargetWeak.Get(), DeltaTime);
	}
}

void ATargetManager::MoveTargetAlongSpline(AEyeTarget* InTargetActor, const float InDeltaTime)
{
	if (!ensureAlways(InTargetActor) || !ensureAlways(TargetSplineComponent)) return;

	// Update distance based on direction
	const float SpeedThisFrame = bMovingForward ? TargetMovementSpeed : -TargetMovementSpeed;
	CurrentDistanceAlongSpline += SpeedThisFrame * InDeltaTime;

	// Check if we've reached either end and reverse direction
	if (SplineLength > 0.f){
		// Avoid division by zero or issues if spline is empty
		if (CurrentDistanceAlongSpline >= SplineLength){
			CurrentDistanceAlongSpline = SplineLength; // Clamp to end
			bMovingForward = false; // Reverse direction
		}
		else if (CurrentDistanceAlongSpline <= 0.0f){
			CurrentDistanceAlongSpline = 0.0f; // Clamp to start
			bMovingForward = true; // Reverse direction
		}
	}
	else{
		CurrentDistanceAlongSpline = 0.0f;
	}
	

	// Move without rotating the actor
	const FVector NewLocation = TargetSplineComponent->GetLocationAtDistanceAlongSpline(
		CurrentDistanceAlongSpline, ESplineCoordinateSpace::World);
	// Update EyeTarget's position and rotation
	InTargetActor->SetActorLocation(NewLocation);
}

bool ATargetManager::OnMoveTargetEnabled(bool bMove)
{
	bMovementActive = bMove;
	return true;
}

bool ATargetManager::OnReverseTargetEnabled(bool bReverse)
{
	bMovingForward = !bReverse;
	return true;
}

void ATargetManager::OnTargetSpeedChanged(float InNewSpeed)
{
	TargetMovementSpeed = InNewSpeed;
}

void ATargetManager::OnTargetColorChanged(const FLinearColor& InNewColor)
{
	if (!ensureAlways(EyeTargetWeak.IsValid())) return;

	EyeTargetColor = InNewColor; // Update local property
	EyeTargetWeak->ChangeMaterialColor(InNewColor);

	// Sync the spline color (and make it lighter/tinted)
	UpdateSplineColors(InNewColor);
}

void ATargetManager::UpdateSplineColors(const FLinearColor& InBaseColor)
{
	// Alpha 0.2 means 80% White, 20% BaseColor.
	const FLinearColor TintedColor = FMath::Lerp(FLinearColor::White, InBaseColor, 0.7f);

	// 1. Update Path Procedural Mesh
	if (!PathMaterialInstance && BasePathMaterial){
		PathMaterialInstance = UMaterialInstanceDynamic::Create(BasePathMaterial, this);
		SplinePathProceduralMesh->SetMaterial(0, PathMaterialInstance);
	}
	if (PathMaterialInstance){
		// "Color" or "BaseColor" depending on the material. BasicShapeMaterial often uses BaseColor.
		PathMaterialInstance->SetVectorParameterValue(FName("Color"), TintedColor);
		PathMaterialInstance->SetVectorParameterValue(FName("BaseColor"), TintedColor);
	}

	// 2. Update Spline Points (HISM)
	if (!SplinePointsMaterialInstance && BasePathMaterial){
		SplinePointsMaterialInstance = UMaterialInstanceDynamic::Create(BasePathMaterial, this);
		SplinePointsHISMComponent->SetMaterial(0, SplinePointsMaterialInstance);
	}
	if (SplinePointsMaterialInstance){
		SplinePointsMaterialInstance->SetVectorParameterValue(FName("Color"), TintedColor);
		SplinePointsMaterialInstance->SetVectorParameterValue(FName("BaseColor"), TintedColor);
	}
}

void ATargetManager::OnTargetShapeChanged(const FSoftObjectPath& InShapePath)
{
	// Set the member pointer
	CurrentShapeAssetPtr = TSoftObjectPtr<UTargetShapeDataAsset>(InShapePath);
	if (CurrentShapeAssetPtr.IsNull()) return;
	
	// Request async load
	UGenericUtilFunctions::RequestSoftObjectAsyncLoad(CurrentShapeAssetPtr,TargetShapeLoadHandle,this,&ATargetManager::OnTargetShapeAssetLoaded);
}

void ATargetManager::OnTargetSizeChanged(float InNewSize)
{
	if (!ensureAlways(EyeTargetWeak.IsValid())) return;

	EyeTargetWeak.Get()->OnChangeMeshSize(InNewSize); 
}

void ATargetManager::OnTargetShapeAssetLoaded()
{
	UTargetShapeDataAsset* LoadedShape = CurrentShapeAssetPtr.Get();
	if (ensureAlways(EyeTargetWeak.IsValid()) && ensureAlways(LoadedShape)){
		EyeTargetWeak->UpdateMeshShape(LoadedShape);
	}
	
	if (TargetShapeLoadHandle.IsValid()){
		TargetShapeLoadHandle.Reset();
	}
}


void ATargetManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bMarooned) return;
	
	if (EyeTargetLoadHandle.IsValid()){
		EyeTargetLoadHandle->CancelHandle();
		ensureAlways(EyeTargetLoadHandle->WasCanceled());
		EyeTargetLoadHandle.Reset();
	}
	if (ControlPanelLoadHandle.IsValid()){
		ControlPanelLoadHandle->CancelHandle();
		ensureAlways(ControlPanelLoadHandle->WasCanceled());
		ControlPanelLoadHandle.Reset();
	}
	if (TargetShapeLoadHandle.IsValid()){
		TargetShapeLoadHandle->CancelHandle();
		ensureAlways(TargetShapeLoadHandle->WasCanceled());
		TargetShapeLoadHandle.Reset();
	}
	if (ensureAlways(ControlPanelSlateWindow.IsValid())){
		ControlPanelSlateWindow->RequestDestroyWindow();
		// Explicitly release our strong reference to the Slate widget.
		// Ensure it's destroyed properly before the actor is garbage-collected.
		ControlPanelSlateWindow.Reset();
	}
}