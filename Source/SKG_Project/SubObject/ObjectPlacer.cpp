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
    if (SelectedObject && !LockedObjects.Contains(SelectedObject))
    {
        SelectedObject->SetActorLocation(NewLocation);
    }
}

void AObjectPlacer::SetSelectedObjectRotation(FRotator NewRotation)
{
    if (SelectedObject && !LockedObjects.Contains(SelectedObject))
    {
        SelectedObject->SetActorRotation(NewRotation);
    }
}

void AObjectPlacer::SetSelectedObjectScale(FVector NewScale)
{
    if (SelectedObject && !LockedObjects.Contains(SelectedObject))
    {
        SelectedObject->SetActorScale3D(NewScale);
    }
}

void AObjectPlacer::ToggleLockSelectedObjects()
{
    if (SelectedObjects.Num() == 0)
    {
        return;
    }

    bool bAnyUnlocked = false;
    for (AActor* Obj : SelectedObjects)
    {
        if (!LockedObjects.Contains(Obj))
        {
            bAnyUnlocked = true;
            break;
        }
    }

    ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn());

    for (AActor* Obj : SelectedObjects)
    {
        if (bAnyUnlocked)
        {
            LockedObjects.Add(Obj);
            if (TransformerPawn)
            {
                TransformerPawn->DeselectActor(Obj); // ロックしたらギズモの操作対象から外す
            }
        }
        else
        {
            LockedObjects.Remove(Obj);
            if (TransformerPawn)
            {
                TransformerPawn->SelectActor(Obj, true); // ロック解除したら再度ギズモの対象にする
            }
        }
    }
}

bool AObjectPlacer::IsSelectedObjectLocked() const
{
    return SelectedObject && LockedObjects.Contains(SelectedObject);
}


void AObjectPlacer::FlipSelectedObject()
{
    if (!SelectedObject || LockedObjects.Contains(SelectedObject))
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
    //InputComponent->BindAction("ReplaceObject", IE_Pressed, this, &AObjectPlacer::OnReplaceObjectKeyPressed);
    InputComponent->BindAction("ToggleLock", IE_Pressed, this, &AObjectPlacer::OnToggleLockKeyPressed);
}

void AObjectPlacer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsLeftMouseDown)
    {
        bool bIsLocked = SelectedObject && LockedObjects.Contains(SelectedObject);
        UE_LOG(LogTemp, Warning, TEXT("Tick check - SelectedObject valid: %s, IsLocked: %s"),
            SelectedObject ? TEXT("true") : TEXT("false"),
            bIsLocked ? TEXT("true") : TEXT("false"));

        if (SelectedObject && !LockedObjects.Contains(SelectedObject))
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
            UE_LOG(LogTemp, Warning, TEXT("Object selected: %s"), *HitActor->GetName());
            SelectedObject = HitActor;
            bIsLeftMouseDown = true;

            SelectedObject = HitActor;
            bIsLeftMouseDown = true;

            bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                if (!LockedObjects.Contains(HitActor))
                {
                    TransformerPawn->SelectActor(HitActor, bShiftHeld);
                }
            }

            if (!bShiftHeld)
            {
                // Shiftを押していない = 新規選択なので、以前の選択のハイライトを全部消す
                for (AActor* Obj : SelectedObjects)
                {
                    SetObjectHighlight(Obj, false);
                }
                SelectedObjects.Empty();
            }

            SelectedObjects.AddUnique(HitActor);
            SetObjectHighlight(HitActor, true); // ← ここに移動

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
}

void AObjectPlacer::OnTranslationModeKeyPressed()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetTransformationType(ETransformationType::TT_Translation);
    }
}

void AObjectPlacer::OnRotationModeKeyPressed()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->SetTransformationType(ETransformationType::TT_Rotation);
    }
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

void AObjectPlacer::OnToggleLockKeyPressed()
{
    ToggleLockSelectedObjects();
}


