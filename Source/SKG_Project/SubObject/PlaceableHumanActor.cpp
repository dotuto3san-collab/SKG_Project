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

    if (FallPhase != EFallPhase::None)
    {
        TickFall(DeltaTime);
        return;
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
        return;
    }

    if (NewStatus == EHumanStatus::Danger)
    {
        // 転倒前の動作を覚えておく(起き上がり後に再開するため)
        bResumeProgram = bRunningProgram;
        bResumeWander = bIsWandering;
        SetWandering(false);

        HumanStatus = EHumanStatus::Danger;
        FallPhase = EFallPhase::Falling;
        FallTimer = PlayAnimationOnce(FallAnimation);
    }
    else // Normalに戻す(外部から呼ばれた場合は転倒を中断)
    {
        SetWandering(false);
        FallPhase = EFallPhase::None;
        HumanStatus = EHumanStatus::Normal;
        bWalkAnimInitialized = false;
        SetWalkAnimationPlaying(false);
    }
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

    FallPhase = EFallPhase::None;
    bWalkAnimInitialized = false;

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

float APlaceableHumanActor::PlayAnimationOnce(UAnimationAsset* Anim)
{
    bWalkAnimInitialized = false; // 次のIdle/Walk切替で必ず再生し直す

    USkeletalMeshComponent* SkelMesh = FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelMesh || !Anim)
    {
        return 0.f;
    }

    SkelMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SkelMesh->PlayAnimation(Anim, false); // ループなし
    return Anim->GetPlayLength();
}

void APlaceableHumanActor::TickFall(float DeltaTime)
{
    FallTimer -= DeltaTime;
    if (FallTimer > 0.f)
    {
        return;
    }

    switch (FallPhase)
    {
    case EFallPhase::Falling:
        FallPhase = EFallPhase::Down;
        FallTimer = DownDuration;
        break;

    case EFallPhase::Down:
    {
        USkeletalMeshComponent* Mesh = FindComponentByClass<USkeletalMeshComponent>();

        // "Hips" を含むボーンを探す(見つからなければ HipsBoneName をそのまま使う)
        FName HipsName = HipsBoneName;
        if (Mesh)
        {
            for (int32 i = 0; i < Mesh->GetNumBones(); ++i)
            {
                const FName BoneName = Mesh->GetBoneName(i);
                if (BoneName.ToString().Contains(TEXT("Hips")))
                {
                    HipsName = BoneName;
                    break;
                }
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("[Fall] Mesh=%s Bone=%s (設定値=%s)"),
            Mesh ? *Mesh->GetName() : TEXT("null"), *HipsName.ToString(), *HipsBoneName.ToString());

        // 向きを変える「前」に、転倒終了時の腰のワールド位置を記録
        const FVector FallHips = Mesh ? Mesh->GetBoneLocation(HipsName, EBoneSpaces::WorldSpace) : FVector::ZeroVector;

        AddActorWorldRotation(FRotator(0.f, GetUpYawOffset, 0.f));

        FallPhase = EFallPhase::GettingUp;
        FallTimer = PlayAnimationOnce(GetUpAnimation);

        if (Mesh)
        {
            // 起き上がりの最初のポーズを即時に評価して、腰の位置を測る
            Mesh->TickAnimation(0.f, false);
            Mesh->RefreshBoneTransforms();

            const FVector GetUpHips = Mesh->GetBoneLocation(HipsName, EBoneSpaces::WorldSpace);
            FVector Delta = FallHips - GetUpHips;
            Delta.Z = 0.f;

            UE_LOG(LogTemp, Warning, TEXT("[Fall] FallHips=%s GetUpHips=%s Delta=%s"),
                *FallHips.ToString(), *GetUpHips.ToString(), *Delta.ToString());

            AddActorWorldOffset(Delta);
        }
        break;
    }

    case EFallPhase::GettingUp:
        FallPhase = EFallPhase::None;
        HumanStatus = EHumanStatus::Normal;
        bWalkAnimInitialized = false;
        SetWalkAnimationPlaying(false);
        if (bResumeProgram)
        {
            bRunningProgram = true; // CommandIndex/CommandProgress はそのまま → 続きから
        }
        else if (bResumeWander)
        {
            SetWandering(true);
        }
        // 追加: 再び Min〜Max 秒のランダム時間で転倒するようにタイマーを再セット
        if (bRepeatFall)
        {
            SetDangerTimerEnabled(true);
        }
        break;

    default:
        break;
    }
}
