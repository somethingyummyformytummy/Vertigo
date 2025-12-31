// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SplineDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class VERTIGO_API USplineDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Spline Data")
	FName SplineDataName;
	UPROPERTY(EditAnywhere, Category = "Spline Data")
	TArray<FVector> SplinePointsLocations;
	UPROPERTY(EditAnywhere, Category = "Spline Data")
	TArray<FVector> SplinePointsTangents;

};
