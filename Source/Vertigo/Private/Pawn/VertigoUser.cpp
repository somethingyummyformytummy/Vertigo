// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/VertigoUser.h"

#include "Camera/CameraComponent.h"
#include "Pawn/UserMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AVertigoUser::AVertigoUser()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());
	CameraComponent->SetRelativeLocation(FVector(0, 0, 60));

	MovementComponent = CreateDefaultSubobject<UUserMovementComponent>(TEXT("Movement Component"));
	MovementComponent->SetComponentTickEnabled(true);
	MovementComponent->AddTickPrerequisiteActor(this);
}

void AVertigoUser::SetupInputMappingContext()
{
	if(auto PlayerController = Cast<APlayerController>(GetWorld()->GetFirstPlayerController())){
		if(auto LocalPlayer = PlayerController->GetLocalPlayer()){
			if(auto SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)){
				SubSystem->AddMappingContext(IMC_Input, 0);
			}
			else{
				ensureAlwaysMsgf(false, TEXT("Could not find Subsystem"));
			}
		}
		else{
			ensureAlwaysMsgf(false, TEXT("Could not find LocalPlayer"));
		}
	}
	else{
		ensureAlwaysMsgf(false, TEXT("Could not find PlayerController"));
	}
}

// Called when the game starts or when spawned
void AVertigoUser::BeginPlay()
{
	Super::BeginPlay();

	SetupInputMappingContext();

	// Disable Gravity
	if (ensureAlways(GetCapsuleComponent())){
		GetCapsuleComponent()->SetEnableGravity(false);
	}
	if (ensureAlways(GetMesh())){
		GetMesh()->SetEnableGravity(false);
	}
	if (ensureAlways(GetCharacterMovement())){
		GetCharacterMovement()->GravityScale = 0.f;
	}

	//if PC
	//CameraComponent->bUsePawnControlRotation = true;
}

// Called every frame
void AVertigoUser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AVertigoUser::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	SetupPlayerInputDelegate.Broadcast(PlayerInputComponent);
}

