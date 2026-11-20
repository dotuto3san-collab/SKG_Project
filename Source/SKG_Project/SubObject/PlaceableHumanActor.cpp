#include "PlaceableHumanActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"

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

    if (bRunningProgram)
    {
        TickProgram(DeltaTime);
        return;
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
    if (bEnable && Commands.Num() > 0 && HumanStatus == EHumanStatus::Normal)
    {
        StartProgram();
        return;
    }
    if (!bEnable)
    {
        bRunningProgram = false;
    }

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

    // 歩いている間だけアニメーションを再生する
    SetWalkAnimationPlaying(bIsWandering);
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
    // 人間が配置された直後は歩行アニメーションをfalseにして、停止アニメーションを再生
    SetWalkAnimationPlaying(false);
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

// 旧式(歩行アニメーションのみ、アニメーション再生停止のみの方式)
/*
void APlaceableHumanActor::SetWalkAnimationPlaying(bool bPlay)
{
    USkeletalMeshComponent* SkelMesh = FindComponentByClass<USkeletalMeshComponent>();

    UE_LOG(LogTemp, Warning, TEXT("[HumanAnim] %s Play=%d SkelMesh=%s Anim=%s"),
        *GetName(), bPlay ? 1 : 0,
        SkelMesh ? *SkelMesh->GetName() : TEXT("null"),
        WalkAnimation ? *WalkAnimation->GetName() : TEXT("null"));

    if (!SkelMesh || !WalkAnimation)
    {
        return;
    }

    if (bPlay)
    {
        SkelMesh->PlayAnimation(WalkAnimation, true); // ループ再生
    }
    else
    {
        SkelMesh->Stop();
        SkelMesh->SetPosition(0.f, false); // 最初のポーズに戻す
    }
}
*/

// 新式(アニメーションをIdleとWalkに切り替える方式)
void APlaceableHumanActor::SetWalkAnimationPlaying(bool bPlay)
{
    // すでに同じ状態なら再生し直さない
    if (bWalkAnimInitialized && bWalkAnimPlaying == bPlay)
    {
        return;
    }

    USkeletalMeshComponent* SkelMesh = FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelMesh)
    {
        return;
    }

    UAnimationAsset* NewAnim = bPlay ? WalkAnimation : IdleAnimation;
    if (!NewAnim)
    {
        return;
    }

    SkelMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SkelMesh->PlayAnimation(NewAnim, true);

    bWalkAnimPlaying = bPlay;
    bWalkAnimInitialized = true;
}

void APlaceableHumanActor::SaveEditorTransform()
{
    EditorTransform = GetActorTransform();
    bHasEditorTransform = true;
}

void APlaceableHumanActor::RestoreEditorTransform()
{
    if (!bHasEditorTransform) return;

    SetWandering(false);
    SetActorTransform(EditorTransform);

    HumanStatus = EHumanStatus::Normal;
    OriginalRotation = EditorTransform.Rotator();
    HomeLocation = EditorTransform.GetLocation();
    bHasEditorTransform = false;
}

void APlaceableHumanActor::StartProgram()
{
    CommandIndex = 0;
    CommandProgress = 0.f;
    bRunningProgram = Commands.Num() > 0;
    SetWalkAnimationPlaying(false);
}

void APlaceableHumanActor::AddCommand(EHumanCommandType Type, float Value)
{
    FHumanCommand Cmd;
    Cmd.Type = Type;
    Cmd.Value = Value;
    Commands.Add(Cmd);
}

void APlaceableHumanActor::SetCommandValue(int32 Index, float Value)
{
    if (Commands.IsValidIndex(Index))
    {
        Commands[Index].Value = Value;
    }
}

void APlaceableHumanActor::RemoveCommand(int32 Index)
{
    if (Commands.IsValidIndex(Index))
    {
        Commands.RemoveAt(Index);
    }
}

void APlaceableHumanActor::ClearCommands()
{
    Commands.Empty();
}

bool APlaceableHumanActor::TickProgram(float DeltaTime)
{
    if (!Commands.IsValidIndex(CommandIndex))
    {
        // 最後まで到達 → 自動で立ち止まる
        bRunningProgram = false;
        SetWalkAnimationPlaying(false);
        return true;
    }

    const FHumanCommand& Cmd = Commands[CommandIndex];
    bool bDone = false;

    switch (Cmd.Type)
    {
    case EHumanCommandType::Walk:
    {
        const float Total = Cmd.Value * StepLength;
        const float Move = FMath::Min(WalkSpeed * DeltaTime, Total - CommandProgress);
        SetActorLocation(GetActorLocation() + GetActorForwardVector() * Move);
        CommandProgress += Move;
        SetWalkAnimationPlaying(true);
        bDone = CommandProgress >= Total - KINDA_SMALL_NUMBER;
        break;
    }
    case EHumanCommandType::Wait:
        SetWalkAnimationPlaying(false);
        CommandProgress += DeltaTime;
        bDone = CommandProgress >= Cmd.Value;
        break;
    case EHumanCommandType::Turn:
    {
        const float TurnSpeed = 90.f; // 度/秒
        const float Total = FMath::Abs(Cmd.Value);
        const float Step = FMath::Min(TurnSpeed * DeltaTime, Total - CommandProgress);
        AddActorWorldRotation(FRotator(0.f, FMath::Sign(Cmd.Value) * Step, 0.f));
        CommandProgress += Step;
        SetWalkAnimationPlaying(false);
        bDone = CommandProgress >= Total - KINDA_SMALL_NUMBER;
        break;
    }
    }

    if (bDone)
    {
        ++CommandIndex;
        CommandProgress = 0.f;
    }
    return false;
}
