// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TargetShapeDataAsset.generated.h"

UCLASS()
class VERTIGO_API UTargetShapeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape Data")
	TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape Data")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape Data")
	TArray<FVector> Normals;

	// Optional
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape Data")
	TArray<FVector2D> UVs;

	// Optional: Vertices for a simplified convex collision shape.
	// If empty, complex collision (per-poly) will be used if enabled on the mesh component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape Data")
	TArray<FVector> ConvexHullVertices;
};
