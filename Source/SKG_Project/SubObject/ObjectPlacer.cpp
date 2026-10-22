#include "ObjectPlacer.h"
#include "TransformerPawn.h"
#include "Engine/Engine.h"
#include "RedoUndoButton.h"
#include "Components/Widget.h"

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
        ClearGizmoSelection();
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

void AObjectPlacer::CycleAlignSnapMode()
{
    switch (AlignSnapMode)
    {
    case EAlignSnapMode::Off:   SetAlignSnapMode(EAlignSnapMode::Auto);  break;
    case EAlignSnapMode::Auto:  SetAlignSnapMode(EAlignSnapMode::AxisX); break;
    case EAlignSnapMode::AxisX: SetAlignSnapMode(EAlignSnapMode::AxisY); break;
    default:                    SetAlignSnapMode(EAlignSnapMode::Off);   break;
    }
}

void AObjectPlacer::SetAlignSnapMode(EAlignSnapMode NewMode)
{
    const bool bWasOff = (AlignSnapMode == EAlignSnapMode::Off);
    const bool bIsOff = (NewMode == EAlignSnapMode::Off);

    if (bWasOff && !bIsOff)
    {
        // Off以外に切り替える瞬間: 今のグリッド設定を覚えてからオフにする
        bSnapToGridBeforeAlign = bSnapToGrid;
        bSnapToGrid = false;
    }
    else if (!bWasOff && bIsOff)
    {
        // Offに戻す瞬間: 覚えておいた設定に戻す
        bSnapToGrid = bSnapToGridBeforeAlign;
    }

    ClearGizmoSelection();

    AlignSnapMode = NewMode;

    const TCHAR* ModeName = TEXT("Off");
    switch (AlignSnapMode)
    {
    case EAlignSnapMode::Auto:  ModeName = TEXT("Auto"); break;
    case EAlignSnapMode::AxisX: ModeName = TEXT("X"); break;
    case EAlignSnapMode::AxisY: ModeName = TEXT("Y"); break;
    default: break;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Align Snap: %s"), ModeName));
    }

    if (MainWidgetRef)
    {
        if (UFunction* Func = MainWidgetRef->FindFunction(FName("ChangeSnapButtonView")))
        {
            MainWidgetRef->ProcessEvent(Func, nullptr);
        }
    }
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
    InputComponent->BindAction("CycleAlignSnap", IE_Pressed, this, &AObjectPlacer::OnCycleAlignSnapKeyPressed);
    InputComponent->BindAction("DeleteObject", IE_Pressed, this, &AObjectPlacer::OnDeleteObjectKeyPressed);
    InputComponent->BindAction("ToggleHumanWander", IE_Pressed, this, &AObjectPlacer::OnToggleHumanWanderKeyPressed);
    InputComponent->BindAction("ToggleHumanDangerTimer", IE_Pressed, this, &AObjectPlacer::OnToggleHumanDangerTimerKeyPressed);
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
    // 撮影UIなどの上をクリックしたときは、配置・選択・ギズモ操作を全部止める
    if (IsMouseOverClickBlockWidget())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Click] Blocked by UI"));
        return;
    }

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

        if (HitActor && PlacedObjects.Contains(HitActor))
        {
            SelectedObject = HitActor;
            bIsLeftMouseDown = true;
            DragStartLocation = HitActor->GetActorLocation();
            DragStartTransform = HitActor->GetActorTransform();
            bDragTracking = true;

            bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

            if (!bShiftHeld)
            {
                for (AActor* Obj : SelectedObjects)
                {
                    SetObjectHighlight(Obj, false);
                }
                SelectedObjects.Empty();

                // ★変更①:外側の if を外して1行にする
                ClearGizmoSelection();
            }

            SelectedObjects.AddUnique(HitActor);
            SetObjectHighlight(HitActor, true);

            RefreshGizmoSelectionForLock();

            return;
        }

        if (HitActor && HitActor->GetClass()->GetName().Contains(TEXT("Gizmo")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Gizmo hit: %s"), *HitActor->GetName());
            bIsLeftMouseDown = true;   // ★変更②:重複していた1行を削除

            if (SelectedObject)
            {
                DragStartLocation = SelectedObject->GetActorLocation();
                bDragTracking = true;
            }

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

        // ★変更③:ログ追加と、解除の条件・中身の整理
        UE_LOG(LogTemp, Warning, TEXT("[Click] Hit=%s Selected=%s Num=%d Class=%s"),
            HitActor ? *HitActor->GetName() : TEXT("null"),
            SelectedObject ? *SelectedObject->GetName() : TEXT("null"),
            SelectedObjects.Num(),
            SelectedObjectClass ? *SelectedObjectClass->GetName() : TEXT("null"));

        if (SelectedObject || SelectedObjects.Num() > 0)
        {
            for (AActor* Obj : SelectedObjects)
            {
                SetObjectHighlight(Obj, false);
            }
            SelectedObjects.Empty();
            SelectedObject = nullptr;
            ClearGizmoSelection();
            return;
        }

        // ここから下は変更なし(配置の処理)
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
            ApplyAlignSnap(NewObject);

            URedoUndoButton* UndoComp = nullptr;
            if (APawn* MyPawn = GetPawn())
            {
                UndoComp = MyPawn->FindComponentByClass<URedoUndoButton>();
            }
            if (!UndoComp)
            {
                UndoComp = FindComponentByClass<URedoUndoButton>();
            }

            if (UndoComp)
            {
                UndoComp->RecordSpawnCommand(NewObject);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[UndoSystem] Failed to find RedoUndoButton Component!"));
            }
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

    // 移動モードでオブジェクトを実際に動かした場合のみ、揃える処理を実行
    if (bDragTracking)
    {
        bDragTracking = false;

        /*====================追加==================*/
        // ドラッグ前後の位置が変わっていたら Undo/Redo 履歴に記録
        if (SelectedObject && !SelectedObject->GetActorTransform().Equals(DragStartTransform, 0.1f))
{
    URedoUndoButton* UndoComp = nullptr;
    if (APawn* MyPawn = GetPawn())
    {
        UndoComp = MyPawn->FindComponentByClass<URedoUndoButton>();
    }
    if (!UndoComp)
    {
        UndoComp = FindComponentByClass<URedoUndoButton>();
    }

    if (UndoComp)
    {
        UndoComp->RecordTransformCommand(SelectedObject, DragStartTransform, SelectedObject->GetActorTransform());
    }
}

        if (CurrentTransformMode == ETransformationType::TT_Translation
            && SelectedObject
            && SelectedObjects.Num() == 1
            && !LockedLocationObjects.Contains(SelectedObject)
            && !SelectedObject->GetActorLocation().Equals(DragStartLocation, 1.f))
        {
            ApplyAlignSnap(SelectedObject);
        }
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

        const bool bCurrentlyActive = GizmoActiveObjects.Contains(Obj);

        if (bLockedForCurrentMode && bCurrentlyActive)
        {
            // ロックされていて、今ギズモに登録されている → 外す
            TransformerPawn->DeselectActor(Obj);
            GizmoActiveObjects.Remove(Obj);
        }
        else if (!bLockedForCurrentMode && !bCurrentlyActive)
        {
            // ロックされていなくて、まだギズモに登録されていない(新規選択を含む) → 登録する
            TransformerPawn->SelectActor(Obj, true);
            GizmoActiveObjects.Add(Obj);
        }
        // それ以外(すでに望ましい状態)は何もしない
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

bool AObjectPlacer::ApplyAlignSnap(AActor* TargetActor)
{
    UE_LOG(LogTemp, Warning, TEXT("[AlignSnap] called Target=%s Mode=%d"),
        TargetActor ? *TargetActor->GetName() : TEXT("null"), (int32)AlignSnapMode);

    if (!TargetActor || AlignSnapMode == EAlignSnapMode::Off)
    {
        return false;
    }

    // 一番近いオブジェクトを基準にする
    AActor* Reference = FindNearestPlacedObject(TargetActor);
    if (!Reference)
    {
        return false;
    }

    const FVector Loc = TargetActor->GetActorLocation();
    const FVector RefLoc = Reference->GetActorLocation();
    const float DiffX = FMath::Abs(Loc.X - RefLoc.X);
    const float DiffY = FMath::Abs(Loc.Y - RefLoc.Y);

    UE_LOG(LogTemp, Warning, TEXT("[AlignSnap] Ref=%s DiffX=%.1f DiffY=%.1f Threshold=%.1f"),
        *Reference->GetName(), DiffX, DiffY, AlignSnapThreshold);

    // 揃える軸は1軸だけ(両軸を揃えると基準と同じ位置に重なるため)
    bool bSnapX = false;
    bool bSnapY = false;

    switch (AlignSnapMode)
    {
    case EAlignSnapMode::AxisX:
        bSnapX = (DiffX <= AlignSnapThreshold);
        break;
    case EAlignSnapMode::AxisY:
        bSnapY = (DiffY <= AlignSnapThreshold);
        break;
    case EAlignSnapMode::Auto:
        // ズレが小さいほうの軸を自動で選ぶ
        if (DiffX <= DiffY)
        {
            bSnapX = (DiffX <= AlignSnapThreshold);
        }
        else
        {
            bSnapY = (DiffY <= AlignSnapThreshold);
        }
        break;
    default:
        break;
    }

    if (!bSnapX && !bSnapY)
    {
        return false; // しきい値を超えているので何もしない
    }

    FVector NewLoc = Loc;
    if (bSnapX) NewLoc.X = RefLoc.X;
    if (bSnapY) NewLoc.Y = RefLoc.Y;

    TargetActor->SetActorLocation(NewLoc);
    return true;
}

AActor* AObjectPlacer::FindNearestPlacedObject(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return nullptr;
    }

    const FVector Origin = TargetActor->GetActorLocation();
    AActor* Nearest = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (AActor* Obj : PlacedObjects)
    {
        if (!IsValid(Obj) || Obj == TargetActor)
        {
            continue;
        }

        // XY平面上の距離で比較する(高さは無視)
        const FVector Loc = Obj->GetActorLocation();
        const float dx = Loc.X - Origin.X;
        const float dy = Loc.Y - Origin.Y;
        const float DistSq = dx * dx + dy * dy;

        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Nearest = Obj;
        }
    }

    return Nearest;
}

void AObjectPlacer::OnCycleAlignSnapKeyPressed()
{
    CycleAlignSnapMode();
}

void AObjectPlacer::AddClickBlockWidget(UWidget* Widget)
{
    if (IsValid(Widget))
    {
        ClickBlockWidgets.AddUnique(Widget);
    }
}

void AObjectPlacer::DeleteSelectedObjects()
{
    if (SelectedObjects.Num() == 0)
    {
        return;
    }

    ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn());

    for (AActor* Obj : SelectedObjects)
    {
        if (!IsValid(Obj))
        {
            continue;
        }

        // ギズモの選択対象から外す
        if (TransformerPawn)
        {
            TransformerPawn->DeselectActor(Obj);
        }

        /*====================追加==================*/
        // Undo/Redo 履歴に削除を記録
        URedoUndoButton* UndoComp = nullptr;
        if (APawn* MyPawn = GetPawn())
        {
            UndoComp = MyPawn->FindComponentByClass<URedoUndoButton>();
        }
        if (!UndoComp)
        {
            UndoComp = FindComponentByClass<URedoUndoButton>();
        }

        if (UndoComp)
        {
            UndoComp->RecordDestroyCommand(Obj);
        }

        // 各リスト・セットから取り除く(消えたポインタが残らないように)
        LockedLocationObjects.Remove(Obj);
        LockedRotationObjects.Remove(Obj);
        LockedScaleObjects.Remove(Obj);
        GizmoActiveObjects.Remove(Obj);
    }

    // 選択状態をクリア
    SelectedObjects.Empty();
    SelectedObject = nullptr;

    UpdateHUDText();
}

void AObjectPlacer::ResetForSceneLoad()
{
    // ハイライトを外す(まだ生きているActorだけ)
    for (AActor* Obj : SelectedObjects)
    {
        if (IsValid(Obj))
        {
            SetObjectHighlight(Obj, false);
        }
    }

    // ギズモ側の選択を解除する(GizmoActiveObjectsも空になる)
    // ※Actorを破棄する「前」に呼ぶことが重要
    ClearGizmoSelection();

    SelectedObjects.Empty();
    SelectedObject = nullptr;

    LockedLocationObjects.Empty();
    LockedRotationObjects.Empty();
    LockedScaleObjects.Empty();

    bIsLeftMouseDown = false;
    bDragTracking = false;

    // すでに破棄されたActorを配置済みリストから掃除する
    PlacedObjects.RemoveAll([](AActor* Obj) { return !IsValid(Obj); });

    // Undo/Redoの履歴もクリアする(読み込み前のActorを指した古い履歴を残さない)
    URedoUndoButton* UndoComp = nullptr;
    if (APawn* MyPawn = GetPawn())
    {
        UndoComp = MyPawn->FindComponentByClass<URedoUndoButton>();
    }
    if (!UndoComp)
    {
        UndoComp = FindComponentByClass<URedoUndoButton>();
    }
    if (UndoComp)
    {
        UndoComp->ClearHistory();
    }

    UpdateHUDText();

    UE_LOG(LogTemp, Warning, TEXT("[SceneLoad] Reset done. PlacedObjects=%d"), PlacedObjects.Num());

}

void AObjectPlacer::RegisterLoadedActor(AActor* LoadedActor)
{
    if (!IsValid(LoadedActor))
    {
        return;
    }

    // 破棄済みのものが残っていたら、ここでも掃除しておく
    PlacedObjects.RemoveAll([](AActor* Obj) { return !IsValid(Obj); });

    PlacedObjects.AddUnique(LoadedActor);

    // 新規配置と同じく、白を入れて初期の黒を避ける
    SetObjectColor(LoadedActor, FLinearColor::White);

    UE_LOG(LogTemp, Warning, TEXT("[SceneLoad] Registered %s (PlacedObjects=%d)"),
        *LoadedActor->GetName(), PlacedObjects.Num());
}

void AObjectPlacer::DebugSimulateReload()
{
    // 今ある(非表示でない)オブジェクトの「クラスと位置」を控える = 「保存」のつもり
    struct FSnap
    {
        UClass* Class;
        FTransform Transform;
    };
    TArray<FSnap> Snaps;

    for (AActor* Obj : PlacedObjects)
    {
        if (IsValid(Obj) && !Obj->IsHidden())
        {
            Snaps.Add({ Obj->GetClass(), Obj->GetActorTransform() });
        }
    }

    // ここから「読み込み」のつもり
    ResetForSceneLoad();

    for (AActor* Obj : PlacedObjects)
    {
        if (IsValid(Obj))
        {
            Obj->Destroy();
        }
    }
    PlacedObjects.Empty();

    for (const FSnap& S : Snaps)
    {
        AActor* NewActor = GetWorld()->SpawnActor<AActor>(S.Class, S.Transform);
        RegisterLoadedActor(NewActor);
    }

    UE_LOG(LogTemp, Warning, TEXT("[SceneLoad] DebugSimulateReload done. Count=%d"), Snaps.Num());

}

void AObjectPlacer::OnToggleHumanWanderKeyPressed()
{
    bHumansWandering = !bHumansWandering;

    for (AActor* Obj : PlacedObjects)
    {
        if (!IsValid(Obj))
        {
            continue;
        }

        if (APlaceableHumanActor* Human = Cast<APlaceableHumanActor>(Obj))
        {
            Human->SetWandering(bHumansWandering);
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
            bHumansWandering ? TEXT("Human Wander: ON") : TEXT("Human Wander: OFF"));
    }
}

void AObjectPlacer::OnToggleHumanDangerTimerKeyPressed()
{
    bHumansDangerTimerEnabled = !bHumansDangerTimerEnabled;

    for (AActor* Obj : PlacedObjects)
    {
        if (!IsValid(Obj))
        {
            continue;
        }

        if (APlaceableHumanActor* Human = Cast<APlaceableHumanActor>(Obj))
        {
            Human->SetDangerTimerEnabled(bHumansDangerTimerEnabled);
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            bHumansDangerTimerEnabled ? TEXT("Danger Timer: ON") : TEXT("Danger Timer: OFF"));
    }
}

void AObjectPlacer::OnDeleteObjectKeyPressed()
{
    DeleteSelectedObjects();
}

void AObjectPlacer::ClearGizmoSelection()
{
    if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
    {
        TransformerPawn->DeselectAll();
    }
    GizmoActiveObjects.Empty();
}

bool AObjectPlacer::IsMouseOverClickBlockWidget() const
{
    for (UWidget* W : ClickBlockWidgets)
    {
        if (IsValid(W) && W->IsHovered())
        {
            return true;
        }
    }
    return false;
}
