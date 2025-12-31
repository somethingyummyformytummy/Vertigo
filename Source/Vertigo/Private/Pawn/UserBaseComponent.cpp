// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/UserBaseComponent.h"

#include "Pawn/VertigoUser.h"

// Sets default values for this component's properties
UUserBaseComponent::UUserBaseComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bWantsInitializeComponent = true;
}


// Called when the game starts
void UUserBaseComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UUserBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UUserBaseComponent::InitializeComponent()
{
	Super::InitializeComponent();

	OwningUser = Cast<AVertigoUser>(GetOwner());
	if(!OwningUser){
		ensureAlwaysMsgf(false, TEXT("Could not find Owner"));
		return;
	}

	OwningUser->SetupPlayerInputDelegate.AddUObject(this, &UUserBaseComponent::SetupPlayerInput);
}

