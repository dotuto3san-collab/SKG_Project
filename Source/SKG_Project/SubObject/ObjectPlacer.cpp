#include "ObjectPlacer.h"
#include "TransformerPawn.h"

void AObjectPlacer::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    SetInputMode(FInputModeGameAndUI());
}

void AObjectPlacer::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &AObjectPlacer::OnLeftClick);
    InputComponent->BindAction("LeftClick", IE_Released, this, &AObjectPlacer::OnLeftClickReleased);
    InputComponent->BindAction("ScaleMode", IE_Pressed, this, &AObjectPlacer::OnScaleModeKeyPressed);
    InputComponent->BindAction("TranslationMode", IE_Pressed, this, &AObjectPlacer::OnTranslationModeKeyPressed);
    InputComponent->BindAction("RotationMode", IE_Pressed, this, &AObjectPlacer::OnRotationModeKeyPressed);
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
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        AActor* HitActor = Hit.GetActor();

        // すでに置いてあるオブジェクトをクリックした場合 → 選択
        if (HitActor && PlacedObjects.Contains(HitActor))
        {
            SelectedObject = HitActor;
            bIsLeftMouseDown = true;   // ← ここだけに移動

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->SelectActor(HitActor);
            }
            return;
        }

        if (HitActor && HitActor->GetClass()->GetName().Contains(TEXT("Gizmo")))
        {
            bIsLeftMouseDown = true;

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->MouseTraceByChannel(10000.f, ECC_Visibility, TArray<AActor*>(), false);
            }
            return;
        }

        // 何か選択中の状態で、それ以外をクリックした場合 → 選択解除のみ
        if (SelectedObject)
        {
            SelectedObject = nullptr;

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->DeselectAll();
            }
            return;
        }

        // 何も選択されておらず、何もない場所をクリックした場合のみ → 新規配置
        // (ここでは bIsLeftMouseDown を true にしない)
        FVector PlacementLocation = Hit.Location + FVector(0.f, 0.f, 50.f);
        AActor* NewObject = GetWorld()->SpawnActor<AActor>(ObjectToSpawn, PlacementLocation, FRotator::ZeroRotator);

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
