// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GenericUtilFunctions.generated.h"

/**
 * 
 */
UCLASS()
class VERTIGO_API UGenericUtilFunctions : public UObject
{
	GENERATED_BODY()

public:

	// Goes through array of all the actors in the world and finds the first actor of the actor class specified
	template <typename T>
	static T* FindActorByClass(UWorld* InWorldContext)
	{
		if(!ensure(InWorldContext)) return nullptr;

		for(TActorIterator<T> It(InWorldContext); It; ++It){
			T* FoundActor = Cast<T>(*It);
			if(FoundActor) return FoundActor;
		}

		// Alert that the actor was not found
		ensureAlwaysMsgf(false, TEXT("Failed to find actor of type %s."), *T::StaticClass()->GetName());
		return nullptr;
	}

	template <typename T>
	static TArray<T*> FindActorsByClass(UWorld* InWorldContext)
	{
		if (!ensure(InWorldContext))return TArray<T*>();

		TArray<T*> FoundActors;
		for (TActorIterator<T> It(InWorldContext); It; ++It){
			FoundActors.Add(*It);
		}

		// Alert that the actor was not found
		if(FoundActors.IsEmpty()){
			ensureAlwaysMsgf(false, TEXT("Failed to find actors of type %s."), *T::StaticClass()->GetName());
		}
	
		return FoundActors;
	}

	// Soft Class
	template <typename T, typename UserClass> static void RequestSoftAsyncLoad(TSoftClassPtr<T>& InAssetPtr,TSharedPtr<FStreamableHandle>& InOutHandle, UserClass* InUserObject, typename FStreamableDelegate::TMethodPtr<UserClass> InCallback)
	{
		if (!InAssetPtr.ToSoftObjectPath().IsValid()){
			return;
		}
		
		// Create the delegate internally
		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(InUserObject, InCallback);

		// If the class is already loaded, just run the callback.
		if (InAssetPtr.IsValid()){
			ensureAlways(Delegate.ExecuteIfBound());
			return;
		}

		// Request async load using the generic function
		InOutHandle = LoadSoftAssetClass(InAssetPtr, Delegate);
	}
	// Asynchronously loads specified asset class
	// Useful for assets that are either big or need to be spawned indefinitely
	template <typename T>
	static TSharedPtr<FStreamableHandle> LoadSoftAssetClass(const TSoftClassPtr<T>& InAssetClassPtr, FStreamableDelegate InDelegate)
	{
		FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
		return StreamableManager.RequestAsyncLoad(InAssetClassPtr.ToSoftObjectPath(), InDelegate);
	}

	// Soft Object
	template <typename T, typename UserClass>
	static void RequestSoftObjectAsyncLoad(TSoftObjectPtr<T>& InAssetPtr, TSharedPtr<FStreamableHandle>& InOutHandle, UserClass* InUserObject, typename FStreamableDelegate::TMethodPtr<UserClass> InCallback)
	{
		// Check if the Soft Pointer itself points to a valid path
		if (!InAssetPtr.ToSoftObjectPath().IsValid()){
			UE_LOG(LogTemp, Warning, TEXT("RequestSoftObjectAsyncLoad: SoftObjectPtr is invalid (path is empty or null)."));
			return;
		}

		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(InUserObject, InCallback);

		// 1. If the object is already loaded (Get() returns valid pointer), just run the callback.
		if (InAssetPtr.Get()){
			ensureAlways(Delegate.ExecuteIfBound());
			return;
		}
    
		// 2. If it's already loading, don't request again
		if (InOutHandle.IsValid() && InOutHandle->IsLoadingInProgress()){
			return;
		}

		// 3. Request async load
		InOutHandle = LoadSoftAssetObject(InAssetPtr, Delegate);
	}
	
	// Asynchronously loads specified asset object
	template <typename T>
	static TSharedPtr<FStreamableHandle> LoadSoftAssetObject(const TSoftObjectPtr<T>& InAssetObjectPtr, FStreamableDelegate InDelegate)
	{
		FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
		return StreamableManager.RequestAsyncLoad(InAssetObjectPtr.ToSoftObjectPath(), InDelegate);
	}

};
