// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VertigoGameMode.generated.h"


DECLARE_MULTICAST_DELEGATE(FPruneTargetManagerSignature)

class ATargetManager;

UCLASS()
class VERTIGO_API AVertigoGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AVertigoGameMode();

	virtual void BeginPlay() override;

	FPruneTargetManagerSignature PruneTargetManagerDelegate;
	
	void SpawnTargetManager(const FTransform& InSpawnTransform);
	
protected:
	UPROPERTY()
	TSubclassOf<ATargetManager> TargetManagerClass;

public:
// Interaction
	UPROPERTY(VisibleAnywhere)
	TWeakObjectPtr<ATargetManager> TargetManager;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FTransform ManagerSpawnTransform = FTransform(
		FQuat(0.0, 0.0, 0.0, 1.0),
		FVector(200.0, 0.0, 70.0), 
FVector(1.0, 1.0, 1.0));
	
};