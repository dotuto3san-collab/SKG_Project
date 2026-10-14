#include "PlaceableHumanActor.h"

APlaceableHumanActor::APlaceableHumanActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void APlaceableHumanActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsWandering)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = CurrentTargetLocation - CurrentLocation;
    ToTarget.Z = 0.f; // 高さは無視して、水平方向だけで判定・移動する

    const float DistanceToTarget = ToTarget.Size();

    // 目的地に十分近づいたら、次の目的地を選び直す
    const float ArrivalThreshold = 20.f; // cm
    if (DistanceToTarget <= ArrivalThreshold)
    {
        PickNewWanderTarget();
        return;
    }

    // 目的地の方向へ、WalkSpeed分だけ進む
    const FVector Direction = ToTarget.GetSafeNormal();
    const FVector NewLocation = CurrentLocation + Direction * WalkSpeed * DeltaTime;
    SetActorLocation(NewLocation);

    // 進んでいる方向を向かせる
    const FRotator NewRotation = Direction.Rotation();
    SetActorRotation(NewRotation);
}

void APlaceableHumanActor::SetHumanStatus(EHumanStatus NewStatus)
{
    if (HumanStatus == NewStatus)
    {
        return; // 既に同じ状態なら何もしない
    }

    // ステータスが切り替わる瞬間、歩行を止める(Tickによる向きの上書きを防ぐため)
    SetWandering(false);

    if (NewStatus == EHumanStatus::Danger)
    {
        OriginalRotation = GetActorRotation();

        FRotator FallenRotation = OriginalRotation;
        FallenRotation.Pitch += 90.f; // 前に倒れる

        SetActorRotation(FallenRotation);
    }
    else // Normalに戻す
    {
        SetActorRotation(OriginalRotation);
    }

    HumanStatus = NewStatus;
}

void APlaceableHumanActor::SetWandering(bool bEnable)
{
    // Danger状態(倒れている)のときは歩かせない
    if (bEnable && HumanStatus != EHumanStatus::Normal)
    {
        return;
    }

    bIsWandering = bEnable;

    if (bIsWandering)
    {
        PickNewWanderTarget();
    }
}

void APlaceableHumanActor::BeginPlay()
{
    Super::BeginPlay();
    HomeLocation = GetActorLocation();
}

void APlaceableHumanActor::PickNewWanderTarget()
{
    // 円の中からランダムな1点を選ぶ(半径・角度をそれぞれランダムにする方式)
    const float RandomAngle = FMath::RandRange(0.f, 360.f);
    const float RandomRadius = FMath::RandRange(0.f, WanderRadius);

    const float OffsetX = RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle));
    const float OffsetY = RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle));

    CurrentTargetLocation = HomeLocation + FVector(OffsetX, OffsetY, 0.f);
}
