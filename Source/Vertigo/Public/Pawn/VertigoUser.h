// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VertigoUser.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSetupPlayerInputDelegateSignature, UInputComponent*)

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class UUserMovementComponent;

UCLASS()
class VERTIGO_API AVertigoUser : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVertigoUser();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InPlayerInputComponent) override;

	FSetupPlayerInputDelegateSignature SetupPlayerInputDelegate;

	UPROPERTY(EditAnywhere, Category = "User Settings | Inputs")
	UInputMappingContext* IMC_Input;
	UPROPERTY(EditAnywhere, Category = "User Settings | Inputs")
	UInputAction* IA_UserMove;
	//PC Port
	UPROPERTY(EditAnywhere, Category = "User Settings | Inputs")
	UInputAction* IA_UserPCLook;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UUserMovementComponent> MovementComponent;

private:
	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;

	void SetupInputMappingContext();
};
