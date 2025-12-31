// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawn/UserBaseComponent.h"
#include "InputActionValue.h"
#include "UserMovementComponent.generated.h"


UCLASS()
class VERTIGO_API UUserMovementComponent : public UUserBaseComponent
{
	GENERATED_BODY()

public:
	UUserMovementComponent();
	
	virtual void SetupPlayerInput(class UInputComponent* InPlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
	float MoveSpeed = 600;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
	float PCLookSpeed = 1;

protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	UFUNCTION()
	void Move(const FInputActionValue& InValues);
	UFUNCTION()
	void Look(const FInputActionValue& InValues);
};

