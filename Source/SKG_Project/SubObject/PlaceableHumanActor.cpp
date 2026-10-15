#include "PlaceableHumanActor.h"

APlaceableHumanActor::APlaceableHumanActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void APlaceableHumanActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ランダム時間でDangerになるタイマー処理(歩行の有無に関わらず動く)
    if (bDangerTimerEnabled)
    {
        TimeUntilDanger -= DeltaTime;
        if (TimeUntilDanger <= 0.f)
        {
            bDangerTimerEnabled = false; // 一度発動したら止める(繰り返さない)
            SetHumanStatus(EHumanStatus::Danger);
        }
    }

    // 歩行処理(bIsWanderingがfalseなら、ここから先は実行しない)
    if (!bIsWandering)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = CurrentTargetLocation - CurrentLocation;
    ToTarget.Z = 0.f;

    const float DistanceToTarget = ToTarget.Size();

    const float ArrivalThreshold = 20.f;
    if (DistanceToTarget <= ArrivalThreshold)
    {
        PickNewWanderTarget();
        return;
    }

    const FVector Direction = ToTarget.GetSafeNormal();
    const FVector NewLocation = CurrentLocation + Direction * WalkSpeed * DeltaTime;
    SetActorLocation(NewLocation);

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

void APlaceableHumanActor::SetDangerTimerEnabled(bool bEnable)
{
    // すでにDanger状態の人には、新しくタイマーを仕掛けない
    if (bEnable && HumanStatus != EHumanStatus::Normal)
    {
        return;
    }

    bDangerTimerEnabled = bEnable;

    if (bDangerTimerEnabled)
    {
        TimeUntilDanger = FMath::RandRange(MinTimeToDanger, MaxTimeToDanger);
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
