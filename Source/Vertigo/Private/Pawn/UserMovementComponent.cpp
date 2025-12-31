// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawn/UserMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Pawn/VertigoUser.h"

UUserMovementComponent::UUserMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// B/c Movement has to happen before any physics calculation
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
}

void UUserMovementComponent::SetupPlayerInput(class UInputComponent* InPlayerInputComponent)
{
	Super::SetupPlayerInput(InPlayerInputComponent);

	if(auto InputSystem = CastChecked<UEnhancedInputComponent>(InPlayerInputComponent)){
		InputSystem->BindAction(OwningUser->IA_UserMove, ETriggerEvent::Triggered, this, &UUserMovementComponent::Move);
		InputSystem->BindAction(OwningUser->IA_UserPCLook, ETriggerEvent::Triggered, this, &UUserMovementComponent::Look);
	}
}


void UUserMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UUserMovementComponent::Move(const FInputActionValue& InValues)
{
	if(!OwningUser) return;

	FVector2D MovementVector = InValues.Get<FVector2D>();
	OwningUser->AddMovementInput(OwningUser->GetActorForwardVector(), MovementVector.X * MoveSpeed);
	OwningUser->AddMovementInput(OwningUser->GetActorRightVector(), MovementVector.Y * MoveSpeed);
}

void UUserMovementComponent::Look(const FInputActionValue& InValues)
{
	if(!OwningUser) return;

	FVector2D LookVector = InValues.Get<FVector2D>();
	OwningUser->AddControllerYawInput(LookVector.X * PCLookSpeed);
	OwningUser->AddControllerPitchInput(LookVector.Y * PCLookSpeed);
}
