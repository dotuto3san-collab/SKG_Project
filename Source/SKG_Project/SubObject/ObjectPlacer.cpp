#include "ObjectPlacer.h"
#include "TransformerPawn.h"

FVector AObjectPlacer::GetSelectedObjectLocation() const
{
    if (SelectedObject)
    {
        return SelectedObject->GetActorLocation();
    }
    return FVector::ZeroVector;
}

FRotator AObjectPlacer::GetSelectedObjectRotation() const
{
    if (SelectedObject)
    {
        return SelectedObject->GetActorRotation();
    }
    return FRotator::ZeroRotator;
}

FVector AObjectPlacer::GetSelectedObjectScale() const
{
    if (SelectedObject)
    {
        return SelectedObject->GetActorScale3D();
    }
    return FVector::OneVector;
}

void AObjectPlacer::ArrangeSelectedObjectsHorizontally()
{
    if (SelectedObjects.Num() < 2)
    {
        return;
    }

    // X座標が小さい順に並び替え
    TArray<AActor*> SortedObjects = SelectedObjects;
    SortedObjects.Sort([](const AActor& A, const AActor& B)
        {
            return A.GetActorLocation().X < B.GetActorLocation().X;
        });

    // 一番左のオブジェクトを基準にする
    FVector BaseLocation = SortedObjects[0]->GetActorLocation();

    for (int32 i = 0; i < SortedObjects.Num(); i++)
    {
        FVector NewLocation = BaseLocation;
        NewLocation.X += GridSnapSize * i;

        SortedObjects[i]->SetActorLocation(NewLocation);
    }
}

void AObjectPlacer::SetSelectedObjectLocation(FVector NewLocation)
{
    if (SelectedObject)
    {
        SelectedObject->SetActorLocation(NewLocation);
    }
}

void AObjectPlacer::SetSelectedObjectRotation(FRotator NewRotation)
{
    if (SelectedObject)
    {
        SelectedObject->SetActorRotation(NewRotation);
    }
}

void AObjectPlacer::SetSelectedObjectScale(FVector NewScale)
{
    if (SelectedObject)
    {
        SelectedObject->SetActorScale3D(NewScale);
    }
}

void AObjectPlacer::ToggleLockSelectedObjectsLocation()
{
    if (SelectedObjects.Num() == 0) return;

    bool bAnyUnlocked = false;
    for (AActor* Obj : SelectedObjects)
    {
        if (!LockedLocationObjects.Contains(Obj)) { bAnyUnlocked = true; break; }
    }

    for (AActor* Obj : SelectedObjects)
    {
        if (bAnyUnlocked) LockedLocationObjects.Add(Obj);
        else LockedLocationObjects.Remove(Obj);
    }

    RefreshGizmoSelectionForLock();
}

void AObjectPlacer::ToggleLockSelectedObjectsRotation()
{
    if (SelectedObjects.Num() == 0) return;

    bool bAnyUnlocked = false;
    for (AActor* Obj : SelectedObjects)
    {
        if (!LockedRotationObjects.Contains(Obj)) { bAnyUnlocked = true; break; }
    }

    for (AActor* Obj : SelectedObjects)
    {
        if (bAnyUnlocked) LockedRotationObjects.Add(Obj);
        else LockedRotationObjects.Remove(Obj);
    }

    RefreshGizmoSelectionForLock();
}

void AObjectPlacer::ToggleLockSelectedObjectsScale()
{
    if (SelectedObjects.Num() == 0) return;

    bool bAnyUnlocked = false;
    for (AActor* Obj : SelectedObjects)
    {
        if (!LockedScaleObjects.Contains(Obj)) { bAnyUnlocked = true; break; }
    }

    for (AActor* Obj : SelectedObjects)
    {
        if (bAnyUnlocked) LockedScaleObjects.Add(Obj);
        else LockedScaleObjects.Remove(Obj);
    }

    RefreshGizmoSelectionForLock();
}

bool AObjectPlacer::IsSelectedObjectLocationLocked() const
{
    return SelectedObject && LockedLocationObjects.Contains(SelectedObject);
}

bool AObjectPlacer::IsSelectedObjectRotationLocked() const
{
    return SelectedObject && LockedRotationObjects.Contains(SelectedObject);
}

bool AObjectPlacer::IsSelectedObjectScaleLocked() const
{
    return SelectedObject && LockedScaleObjects.Contains(SelectedObject);
}

void AObjectPlacer::ReplaceSelectedObjects()
{
    if (SelectedObjects.Num() == 0 || !SelectedObjectClass)
    {
        return;
    }

    // 先に選択解除してから、破棄・生成する
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->DeselectAll();
    }

    TArray<AActor*> NewSelection;

    for (AActor* OldActor : SelectedObjects)
    {
        if (!OldActor) continue;

        FVector Location = OldActor->GetActorLocation();
        FRotator Rotation = OldActor->GetActorRotation();
        FVector Scale = OldActor->GetActorScale3D();

        PlacedObjects.Remove(OldActor);
        LockedLocationObjects.Remove(OldActor);
        LockedRotationObjects.Remove(OldActor);
        LockedScaleObjects.Remove(OldActor);
        OldActor->Destroy();

        AActor* NewActor = GetWorld()->SpawnActor<AActor>(SelectedObjectClass, Location, Rotation);
        if (NewActor)
        {
            NewActor->SetActorScale3D(Scale);
            PlacedObjects.Add(NewActor);
            SetObjectColor(NewActor, FLinearColor::White);
            NewSelection.Add(NewActor);
        }
    }

    SelectedObjects = NewSelection;
    SelectedObject = SelectedObjects.Num() > 0 ? SelectedObjects.Last() : nullptr;

    // 新しく生成したオブジェクトのハイライトを付ける
    for (AActor* Obj : SelectedObjects)
    {
        SetObjectHighlight(Obj, true);
    }

    // 選択とロック状態の反映をまとめて行う
    RefreshGizmoSelectionForLock();
}


void AObjectPlacer::FlipSelectedObject()
{
    if (!SelectedObject)
    {
        return;
    }

    FRotator CurrentRotation = SelectedObject->GetActorRotation();
    CurrentRotation.Yaw += 180.f;
    SelectedObject->SetActorRotation(CurrentRotation);
}

void AObjectPlacer::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    SetInputMode(FInputModeGameAndUI());

    // カメラの向きをControlRotationに同期させる
    if (APawn* MyPawn = GetPawn())
    {
        SetControlRotation(MyPawn->GetActorRotation());
    }

    if (bShowDebugHUD && HUDWidgetClass)
    {
        HUDWidgetInstance = CreateWidget<UObjectHUDWidget>(this, HUDWidgetClass);
        if (HUDWidgetInstance)
        {
            HUDWidgetInstance->AddToViewport();
        }
    }

    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetSnappingValue(ETransformationType::TT_Translation, GridSnapSize);
        TransformerPawn->SetSnappingEnabled(ETransformationType::TT_Translation, bSnapToGrid);
    }

    if (TestAsset)
    {
        SetSelectedObject(TestAsset);
    }
}

void AObjectPlacer::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &AObjectPlacer::OnLeftClick);
    InputComponent->BindAction("LeftClick", IE_Released, this, &AObjectPlacer::OnLeftClickReleased);
    InputComponent->BindAction("ScaleMode", IE_Pressed, this, &AObjectPlacer::OnScaleModeKeyPressed);
    InputComponent->BindAction("TranslationMode", IE_Pressed, this, &AObjectPlacer::OnTranslationModeKeyPressed);
    InputComponent->BindAction("RotationMode", IE_Pressed, this, &AObjectPlacer::OnRotationModeKeyPressed);
    InputComponent->BindAction("SwitchObject", IE_Pressed, this, &AObjectPlacer::OnSwitchObjectKeyPressed);
    InputComponent->BindAction("FlipObject", IE_Pressed, this, &AObjectPlacer::OnFlipObjectKeyPressed);
    InputComponent->BindAction("ArrangeObjects", IE_Pressed, this, &AObjectPlacer::OnArrangeObjectsKeyPressed);
    InputComponent->BindAction("ToggleHumanStatus", IE_Pressed, this, &AObjectPlacer::OnToggleHumanStatusKeyPressed);
    InputComponent->BindAction("ReplaceObject", IE_Pressed, this, &AObjectPlacer::OnReplaceObjectKeyPressed);
    InputComponent->BindAction("ToggleLockLocation", IE_Pressed, this, &AObjectPlacer::OnToggleLockLocationKeyPressed);
    InputComponent->BindAction("ToggleLockRotation", IE_Pressed, this, &AObjectPlacer::OnToggleLockRotationKeyPressed);
    InputComponent->BindAction("ToggleLockScale", IE_Pressed, this, &AObjectPlacer::OnToggleLockScaleKeyPressed);
}

void AObjectPlacer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsLeftMouseDown)
    {
        if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
        {
            FVector WorldLocation, WorldDirection;
            if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
            {
                FVector LookingVector = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Vector() : WorldDirection;

                TransformerPawn->UpdateTransform(LookingVector, WorldLocation, WorldDirection);
            }
        }
    }
}

void AObjectPlacer::OnLeftClick()
{
    FHitResult Hit;
    FCollisionQueryParams Params;

    if (SelectedObject)
    {
        Params.AddIgnoredActor(SelectedObject);
    }

    FVector WorldLocation, WorldDirection;
    if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector TraceEnd = WorldLocation + WorldDirection * 10000.f;
        GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params);
    }

    if (Hit.bBlockingHit)
    {
        AActor* HitActor = Hit.GetActor();

        // ↓ ここを元に戻す
        if (HitActor && PlacedObjects.Contains(HitActor))
        {
            SelectedObject = HitActor;
            bIsLeftMouseDown = true;

            bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

            if (!bShiftHeld)
            {
                for (AActor* Obj : SelectedObjects)
                {
                    SetObjectHighlight(Obj, false);
                }
                SelectedObjects.Empty();

                // Shiftを押していない新規選択の場合、TransformerPawn側の選択もクリアする
                if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
                {
                    TransformerPawn->DeselectAll();
                }
            }

            SelectedObjects.AddUnique(HitActor);
            SetObjectHighlight(HitActor, true);

            RefreshGizmoSelectionForLock(); // ここで選択(SelectActor)をまとめて行う

            return;
        }

        if (HitActor && HitActor->GetClass()->GetName().Contains(TEXT("Gizmo")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Gizmo hit: %s"), *HitActor->GetName());
    bIsLeftMouseDown = true;
            bIsLeftMouseDown = true;

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TArray<AActor*> IgnoreList;
                if (SelectedObject)
                {
                    IgnoreList.Add(SelectedObject);
                }

                TransformerPawn->MouseTraceByChannel(10000.f, ECC_Visibility, IgnoreList, false);
            }
            return;
        }


        if (SelectedObject)
        {
            for (AActor* Obj : SelectedObjects)
            {
                SetObjectHighlight(Obj, false);
            }
            SelectedObjects.Empty();
            SelectedObject = nullptr;

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->DeselectAll();
            }
            return;
        }

        FVector PlacementLocation = Hit.Location + FVector(0.f, 0.f, 50.f);

        if (bSnapToGrid)
        {
            PlacementLocation = FVector(
                FMath::GridSnap(PlacementLocation.X, GridSnapSize),
                FMath::GridSnap(PlacementLocation.Y, GridSnapSize),
                PlacementLocation.Z
            );
        }

        AActor* NewObject = GetWorld()->SpawnActor<AActor>(SelectedObjectClass, PlacementLocation, FRotator::ZeroRotator);

        if (NewObject)
        {
            PlacedObjects.Add(NewObject);
            SetObjectColor(NewObject, FLinearColor::White);
        }
    }

}

void AObjectPlacer::OnLeftClickReleased()
{
    bIsLeftMouseDown = false;

    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->ClearDomain();
    }
}

void AObjectPlacer::OnScaleModeKeyPressed()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetTransformationType(ETransformationType::TT_Scale);
    }
    CurrentTransformMode = ETransformationType::TT_Scale;
    RefreshGizmoSelectionForLock();
}

void AObjectPlacer::OnTranslationModeKeyPressed()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetTransformationType(ETransformationType::TT_Translation);
    }
    CurrentTransformMode = ETransformationType::TT_Translation;
    RefreshGizmoSelectionForLock();
}

void AObjectPlacer::OnRotationModeKeyPressed()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetTransformationType(ETransformationType::TT_Rotation);
    }
    CurrentTransformMode = ETransformationType::TT_Rotation;
    RefreshGizmoSelectionForLock();
}

void AObjectPlacer::SetObjectColor(AActor* TargetActor, FLinearColor Color)
{
    if (!TargetActor) return;

    if (UStaticMeshComponent* Mesh = TargetActor->FindComponentByClass<UStaticMeshComponent>())
    {
        UMaterialInstanceDynamic* DynMat = Mesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMat)
        {
            DynMat->SetVectorParameterValue(FName("Color"), Color);
        }
    }
}

void AObjectPlacer::SetObjectHighlight(AActor* TargetActor, bool bHighlighted)
{
    if (!TargetActor) return;

    if (UStaticMeshComponent* Mesh = TargetActor->FindComponentByClass<UStaticMeshComponent>())
    {
        Mesh->SetOverlayMaterial(bHighlighted ? HighlightMaterial : nullptr);
    }
}

void AObjectPlacer::SetSelectedObject(UPlaceableObjectAsset* ObjectAsset)
{
    if (ObjectAsset)
    {
        SelectedObjectClass = ObjectAsset->ActorClass;
        SelectedObjectCategory = ObjectAsset->Category;
    }

    UpdateHUDText();
}

void AObjectPlacer::OnSwitchObjectKeyPressed()
{
    if (SelectedObjectClass == TestAsset->ActorClass)
    {
        SetSelectedObject(TestAsset2);
    }
    else if (SelectedObjectClass == TestAsset2->ActorClass)
    {
        SetSelectedObject(TestAsset3);
    }
    else
    {
        SetSelectedObject(TestAsset);
    }
}

void AObjectPlacer::UpdateHUDText()
{
    if (!bShowDebugHUD || !HUDWidgetInstance)
    {
        return;
    }

    FText DisplayText = FText::FromString(TEXT("None"));

    if (SelectedObjectClass)
    {
        DisplayText = GetCategoryDisplayText(SelectedObjectCategory);
    }

    HUDWidgetInstance->SetCurrentObjectText(DisplayText);
}

FText AObjectPlacer::GetCategoryDisplayText(EObjectCategory Category) const
{
    switch (Category)
    {
    case EObjectCategory::Furniture:
        return FText::FromString(TEXT("Furniture"));
    case EObjectCategory::Machine:
        return FText::FromString(TEXT("Machine"));
    case EObjectCategory::Human:
        return FText::FromString(TEXT("Human"));
    default:
        return FText::FromString(TEXT("None"));
    }
}

void AObjectPlacer::OnFlipObjectKeyPressed()
{
    if (!SelectedObject)
    {
        return;
    }

    FRotator CurrentRotation = SelectedObject->GetActorRotation();
    CurrentRotation.Yaw += 180.f;
    SelectedObject->SetActorRotation(CurrentRotation);
}

void AObjectPlacer::OnArrangeObjectsKeyPressed()
{
    ArrangeSelectedObjectsHorizontally();
}

void AObjectPlacer::OnToggleHumanStatusKeyPressed()
{
    if (!SelectedObject)
    {
        return;
    }

    if (APlaceableHumanActor* HumanActor = Cast<APlaceableHumanActor>(SelectedObject))
    {
        EHumanStatus NewStatus = (HumanActor->GetHumanStatus() == EHumanStatus::Normal)
            ? EHumanStatus::Danger
            : EHumanStatus::Normal;

        HumanActor->SetHumanStatus(NewStatus);
    }
}

void AObjectPlacer::RefreshGizmoSelectionForLock()
{
    ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn());
    if (!TransformerPawn) return;

    for (AActor* Obj : SelectedObjects)
    {
        bool bLockedForCurrentMode = false;

        switch (CurrentTransformMode)
        {
        case ETransformationType::TT_Translation:
            bLockedForCurrentMode = LockedLocationObjects.Contains(Obj);
            break;
        case ETransformationType::TT_Rotation:
            bLockedForCurrentMode = LockedRotationObjects.Contains(Obj);
            break;
        case ETransformationType::TT_Scale:
            bLockedForCurrentMode = LockedScaleObjects.Contains(Obj);
            break;
        default:
            break;
        }

        UE_LOG(LogTemp, Warning, TEXT("RefreshGizmo - Obj: %s, LockedForCurrentMode: %s"),
            *Obj->GetName(), bLockedForCurrentMode ? TEXT("true") : TEXT("false"));


        if (bLockedForCurrentMode)
        {
            TransformerPawn->DeselectActor(Obj);
        }
        else
        {
            TransformerPawn->SelectActor(Obj, true);
        }
    }
}

void AObjectPlacer::OnToggleLockLocationKeyPressed()
{
    ToggleLockSelectedObjectsLocation();
}

void AObjectPlacer::OnToggleLockRotationKeyPressed()
{
    ToggleLockSelectedObjectsRotation();
}

void AObjectPlacer::OnToggleLockScaleKeyPressed()
{
    ToggleLockSelectedObjectsScale();
}

void AObjectPlacer::OnReplaceObjectKeyPressed()
{
    ReplaceSelectedObjects();
}


