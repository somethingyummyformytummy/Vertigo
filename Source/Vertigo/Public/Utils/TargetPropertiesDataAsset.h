// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TargetPropertiesDataAsset.generated.h"

UENUM(BlueprintType)
enum class ETargetPropertyType : uint8
{
	SHAPE,
	COLOR,
	UNDEFINED
};

UCLASS()
class VERTIGO_API UTargetPropertiesDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Target Properties")
	ETargetPropertyType TargetPropertyType;
	UPROPERTY(EditAnywhere, Category = "Target Properties")
	FName AssetName;
	UPROPERTY(EditAnywhere, Category = "Target Properties", meta = (EditCondition = "TargetPropertyType == ETargetPropertyType::COLOR"))
	FLinearColor TargetColor;
	UPROPERTY(EditAnywhere, Category = "Target Properties", meta = (EditCondition = "TargetPropertyType == ETargetPropertyType::SHAPE"))
	FName TargetShape;
};
