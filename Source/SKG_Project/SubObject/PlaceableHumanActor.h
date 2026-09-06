#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaceableObjectAsset.h"
#include "PlaceableHumanActor.generated.h"

UCLASS()
class SKG_PROJECT_API APlaceableHumanActor : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Human Status")
    void SetHumanStatus(EHumanStatus NewStatus);

    UFUNCTION(BlueprintPure, Category = "Human Status")
    EHumanStatus GetHumanStatus() const { return HumanStatus; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Human Status")
    EHumanStatus HumanStatus = EHumanStatus::Normal;
};
