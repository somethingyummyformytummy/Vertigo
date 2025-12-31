// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/VertigoGameMode.h"

#include "Interaction/TargetManager.h"
#include "Pawn/VertigoUser.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/SlateTypes.h"
#include "Styling/AppStyle.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "Math/Color.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

// Add this include at the top of your VertigoGameMode.cpp
#include "GameFramework/PlayerController.h"

AVertigoGameMode::AVertigoGameMode()
{
    //Set Default Pawn
    //Added _C b/c Blueprint Class
    ConstructorHelpers::FClassFinder<AVertigoUser> TempUser(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/Pawns/BP_BaseVertigoUser.BP_BaseVertigoUser_C'"));
    if (TempUser.Succeeded()){
       DefaultPawnClass = TempUser.Class;
    }
    ConstructorHelpers::FClassFinder<ATargetManager> TempManager(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/Interaction/BP_TargetManager.BP_TargetManager_C'"));
    if (TempManager.Succeeded()){
       TargetManagerClass = TempManager.Class;
    }
}

void AVertigoGameMode::BeginPlay()
{
    Super::BeginPlay();

    SpawnTargetManager(ManagerSpawnTransform);
}

void AVertigoGameMode::SpawnTargetManager(const FTransform& InSpawnTransform)
{
    if (!ensureAlways(GetWorld()) || !ensureAlways(TargetManagerClass)){
       return;
    }
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    TargetManager = GetWorld()->SpawnActor<ATargetManager>(TargetManagerClass, InSpawnTransform, SpawnParams);
    ensureAlways(TargetManager.IsValid());
	if (PruneTargetManagerDelegate.IsBound()){
		PruneTargetManagerDelegate.Broadcast();
	}
}

