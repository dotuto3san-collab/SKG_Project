// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlaceableObjectAsset.h"
#include "ObjectHUDWidget.h"
#include "PlaceableHumanActor.h"
#include "TransformerPawn.h"
#include "ObjectPlacer.generated.h"

UENUM(BlueprintType)
enum class EAlignSnapMode : uint8
{
    Off     UMETA(DisplayName = "Off"),
    Auto    UMETA(DisplayName = "Auto"),   // ズレが小さいほうの軸を自動で選ぶ
    AxisX   UMETA(DisplayName = "X"),      // X座標を基準オブジェクトに揃える
    AxisY   UMETA(DisplayName = "Y")       // Y座標を基準オブジェクトに揃える
};

/**
 * 
 */
UCLASS()
class SKG_PROJECT_API AObjectPlacer : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Object Placement")
    AActor* GetSelectedObject() const { return SelectedObject; }

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    FVector GetSelectedObjectLocation() const;

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    FRotator GetSelectedObjectRotation() const;

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    FVector GetSelectedObjectScale() const;

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void ArrangeSelectedObjectsHorizontally();

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void SetSelectedObjectLocation(FVector NewLocation);

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void SetSelectedObjectRotation(FRotator NewRotation);

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void SetSelectedObjectScale(FVector NewScale);

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void ToggleLockSelectedObjectsLocation();

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void ToggleLockSelectedObjectsRotation();

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void ToggleLockSelectedObjectsScale();

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    bool IsSelectedObjectLocationLocked() const;

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    bool IsSelectedObjectRotationLocked() const;

    UFUNCTION(BlueprintPure, Category = "Object Placement")
    bool IsSelectedObjectScaleLocked() const;

    UFUNCTION(BlueprintCallable, Category = "Object Placement")
    void ReplaceSelectedObjects();

    void FlipSelectedObject();

    UFUNCTION(BlueprintCallable, Category = "Object Placement|Snap")
    void CycleAlignSnapMode();

    UFUNCTION(BlueprintCallable, Category = "Object Placement|Snap")
    void SetAlignSnapMode(EAlignSnapMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Object Placement|Snap")
    EAlignSnapMode GetAlignSnapMode() const { return AlignSnapMode; }

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void OnLeftClick();
    void OnLeftClickReleased();
    void OnScaleModeKeyPressed();
    void OnTranslationModeKeyPressed();
    void OnRotationModeKeyPressed();
    bool bIsLeftMouseDown = false;

    void SetObjectColor(AActor* TargetActor, FLinearColor Color);

    void SetObjectHighlight(AActor* TargetActor, bool bHighlighted);

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<AActor> ObjectToSpawn;

    UPROPERTY()
    TSubclassOf<AActor> SelectedObjectClass;

    UFUNCTION(BlueprintCallable)
    void SetSelectedObject(UPlaceableObjectAsset* ObjectAsset);

    UPROPERTY()
    TArray<AActor*> PlacedObjects;

    UPROPERTY()
    AActor* SelectedObject = nullptr;

    UPROPERTY()
    FLinearColor OriginalColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly)
    UPlaceableObjectAsset* TestAsset;

    UPROPERTY(EditDefaultsOnly)
    UPlaceableObjectAsset* TestAsset2;

    UPROPERTY(EditDefaultsOnly)
    UPlaceableObjectAsset* TestAsset3;

    UFUNCTION()
    void OnSwitchObjectKeyPressed();

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class UUserWidget> ObjectHUDClass;

    UPROPERTY()
    class UObjectHUDWidget* ObjectHUDInstance;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSubclassOf<UObjectHUDWidget> HUDWidgetClass;

    UPROPERTY()
    UObjectHUDWidget* HUDWidgetInstance = nullptr;

    UPROPERTY(EditAnywhere, Category = "HUD")
    bool bShowDebugHUD = true;

    void UpdateHUDText();

    FText GetCategoryDisplayText(EObjectCategory Category) const;

    UPROPERTY()
    EObjectCategory SelectedObjectCategory = EObjectCategory::Furniture;

    UPROPERTY(EditAnywhere, Category = "Snapping")
    bool bSnapToGrid = true;

    UPROPERTY(EditAnywhere, Category = "Snapping")
    float GridSnapSize = 50.f;

    UFUNCTION()
    void OnFlipObjectKeyPressed();

    UPROPERTY()
    TArray<AActor*> SelectedObjects;

    UPROPERTY(EditDefaultsOnly, Category = "Highlight")
    UMaterialInterface* HighlightMaterial;

    UFUNCTION()
    void OnArrangeObjectsKeyPressed();

    UFUNCTION()
    void OnToggleHumanStatusKeyPressed();

    UPROPERTY()
    TSet<AActor*> LockedLocationObjects;

    UPROPERTY()
    TSet<AActor*> LockedRotationObjects;

    UPROPERTY()
    TSet<AActor*> LockedScaleObjects;

    void RefreshGizmoSelectionForLock();

    UFUNCTION()
    void OnToggleLockLocationKeyPressed();

    UFUNCTION()
    void OnToggleLockRotationKeyPressed();

    UFUNCTION()
    void OnToggleLockScaleKeyPressed();

    UPROPERTY()
    ETransformationType CurrentTransformMode = ETransformationType::TT_Translation;

    UFUNCTION()
    void OnReplaceObjectKeyPressed();

    // 揃えるスナップのモード。Off(初期値)のときは従来どおりグリッドスナップが働く
    UPROPERTY(EditAnywhere, Category = "Snapping|Align")
    EAlignSnapMode AlignSnapMode = EAlignSnapMode::Off;

    // この距離(cm)以内のズレなら自動で揃える
    UPROPERTY(EditAnywhere, Category = "Snapping|Align", meta = (ClampMin = "0.0"))
    float AlignSnapThreshold = 30.f;

    // targetを一番近いオブジェクトに揃える。実際に動かしたらtrueを返す
    bool ApplyAlignSnap(AActor* TargetActor);

    // XY平面上で一番近い配置済みオブジェクトを探す(自分自身は除く)
    AActor* FindNearestPlacedObject(AActor* TargetActor) const;

    UFUNCTION()
    void OnCycleAlignSnapKeyPressed();
};
