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

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<AActor> ObjectToSpawn;
};
