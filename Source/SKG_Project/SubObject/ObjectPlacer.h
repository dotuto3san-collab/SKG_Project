// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlaceableObjectAsset.h"
#include "ObjectHUDWidget.h"
#include "ObjectPlacer.generated.h"

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

    void FlipSelectedObject();

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

    //UPROPERTY(EditDefaultsOnly)
    //TSubclassOf<AActor> ObjectToSpawn;

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
};
