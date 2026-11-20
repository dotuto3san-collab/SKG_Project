#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaceableObjectAsset.h"
#include "PlaceableHumanActor.generated.h"

UENUM(BlueprintType)
enum class EHumanCommandType : uint8
{
    Walk UMETA(DisplayName = "歩く"),
    Wait UMETA(DisplayName = "止まる"),
    Turn UMETA(DisplayName = "回る")
};

USTRUCT(BlueprintType)
struct FHumanCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHumanCommandType Type = EHumanCommandType::Walk;

    // Walk=歩数 / Wait=秒 / Turn=度(マイナスで逆回り)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 1.f;
};


class UAnimationAsset;

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

    UFUNCTION(BlueprintCallable, Category = "Danger Timer")
    void SetDangerTimerEnabled(bool bEnable);

    UFUNCTION(BlueprintPure, Category = "Danger Timer")
    bool IsDangerTimerEnabled() const { return bDangerTimerEnabled; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander")
    UAnimationAsset* WalkAnimation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall")
    UAnimationAsset* FallAnimation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall")
    UAnimationAsset* GetUpAnimation = nullptr;

    // 転倒アニメ終了後、倒れたまま待つ秒数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall", meta = (ClampMin = "0.0"))
    float DownDuration = 3.f;

    // 起き上がった後も、再びランダムな時間で転倒を繰り返すか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall")
    bool bRepeatFall = true;

    // 腰ボーン名(Mixamo の場合)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall")
    FName HipsBoneName = TEXT("mixamorig1:Hips");

    // 起き上がり開始時にアクターを回す角度(度)。まず 45 で試し、逆にずれるなら -45
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall")
    float GetUpYawOffset = -45.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander")
    UAnimationAsset* IdleAnimation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wander")
    float SpawnYaw = 90.f;

    // 実行前の状態を保存／復元する
    UFUNCTION(BlueprintCallable, Category = "Wander")
    void SaveEditorTransform();

    UFUNCTION(BlueprintCallable, Category = "Wander")
    void RestoreEditorTransform();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Program")
    TArray<FHumanCommand> Commands;

    // 1歩の長さ(cm)
    UPROPERTY(EditAnywhere, Category = "Program")
    float StepLength = 50.f;

    UFUNCTION(BlueprintCallable, Category = "Program")
    void StartProgram();

    UFUNCTION(BlueprintCallable, Category = "Program")
    void AddCommand(EHumanCommandType Type, float Value);

    UFUNCTION(BlueprintCallable, Category = "Program")
    void SetCommandValue(int32 Index, float Value);

    UFUNCTION(BlueprintCallable, Category = "Program")
    void RemoveCommand(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Program")
    void ClearCommands();

protected:
    virtual void BeginPlay() override;

    void PickNewWanderTarget();

    void SetWalkAnimationPlaying(bool bPlay);

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

    // この機能が有効かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Danger Timer")
    bool bDangerTimerEnabled = false;

    // Dangerになるまでの待ち時間の範囲(秒)。Blueprint側で人ごとに調整可能だが、
    // 同じBP_PlaceObject_Humanから複製されるため、通常は全員共通の値になる
    UPROPERTY(EditAnywhere, Category = "Danger Timer", meta = (ClampMin = "0.0"))
    float MinTimeToDanger = 10.f;

    UPROPERTY(EditAnywhere, Category = "Danger Timer", meta = (ClampMin = "0.0"))
    float MaxTimeToDanger = 60.f;

    // 今、Dangerになるまであと何秒か(カウントダウン)
    UPROPERTY()
    float TimeUntilDanger = 0.f;

    // 今向かっている目的地
    UPROPERTY()
    FVector CurrentTargetLocation = FVector::ZeroVector;

    // 徘徊の中心地点(配置された場所)
    UPROPERTY()
    FVector HomeLocation = FVector::ZeroVector;

    UPROPERTY()
    FTransform EditorTransform;

    UPROPERTY()
    bool bHasEditorTransform = false;

    bool bRunningProgram = false;
    int32 CommandIndex = 0;
    float CommandProgress = 0.f;   // 進んだ距離 / 経過秒 / 回った角度

    bool TickProgram(float DeltaTime);

    bool bWalkAnimPlaying = false;
    bool bWalkAnimInitialized = false;

    enum class EFallPhase : uint8 { None, Falling, Down, GettingUp };
    EFallPhase FallPhase = EFallPhase::None;
    float FallTimer = 0.f;
    bool bResumeProgram = false;
    bool bResumeWander = false;

    void TickFall(float DeltaTime);
    float PlayAnimationOnce(UAnimationAsset* Anim);
};
