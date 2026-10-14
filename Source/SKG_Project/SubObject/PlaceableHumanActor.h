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
    APlaceableHumanActor();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Human Status")
    void SetHumanStatus(EHumanStatus NewStatus);

    UFUNCTION(BlueprintPure, Category = "Human Status")
    EHumanStatus GetHumanStatus() const { return HumanStatus; }

    UFUNCTION(BlueprintCallable, Category = "Wander")
    void SetWandering(bool bEnable);

    UFUNCTION(BlueprintPure, Category = "Wander")
    bool IsWandering() const { return bIsWandering; }

protected:
    virtual void BeginPlay() override;

    void PickNewWanderTarget();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Human Status")
    EHumanStatus HumanStatus = EHumanStatus::Normal;

    UPROPERTY()
    FRotator OriginalRotation;

    // 今、歩き回っている最中かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Wander")
    bool bIsWandering = false;

    // HomeLocationからどのくらいの範囲内を歩き回るか(cm)。Blueprint側で人ごとに調整可能
    UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
    float WanderRadius = 500.f;

    // 歩く速さ(cm/秒)
    UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
    float WalkSpeed = 100.f;

    // 今向かっている目的地
    UPROPERTY()
    FVector CurrentTargetLocation = FVector::ZeroVector;

    // 徘徊の中心地点(配置された場所)
    UPROPERTY()
    FVector HomeLocation = FVector::ZeroVector;
};
