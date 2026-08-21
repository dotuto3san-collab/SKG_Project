// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ObjectPlacer.generated.h"

/**
 * 
 */
UCLASS()
class SKG_PROJECT_API AObjectPlacer : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    UFUNCTION()
    void OnLeftClick();

    void SetObjectColor(AActor* TargetActor, FLinearColor Color);

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<AActor> ObjectToSpawn;

    UPROPERTY()
    TArray<AActor*> PlacedObjects;

    UPROPERTY()
    AActor* SelectedObject = nullptr;

    UPROPERTY()
    FLinearColor OriginalColor = FLinearColor::White;
};
