// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlaceableObjectAsset.generated.h"

UENUM(BlueprintType)
enum class EObjectCategory : uint8
{
    Furniture   UMETA(DisplayName = "â∆ãÔ"),
    Machine     UMETA(DisplayName = "ã@äB"),
    Human       UMETA(DisplayName = "êl")
};

UENUM(BlueprintType)
enum class EHumanStatus : uint8
{
    Normal  UMETA(DisplayName = "Nomal"),
    Danger  UMETA(DisplayName = "Danger")
};

UCLASS()
class SKG_PROJECT_API UPlaceableObjectAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EObjectCategory Category;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<AActor> ActorClass;
};
