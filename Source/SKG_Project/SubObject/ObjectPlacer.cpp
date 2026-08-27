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

        // Å´ Ç±Ç±Çå≥Ç…ñﬂÇ∑
        if (HitActor && PlacedObjects.Contains(HitActor))
        {
            SelectedObject = HitActor;
            bIsLeftMouseDown = true;

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
                TArray<AActor*> IgnoreList;
                if (SelectedObject)
                {
                    IgnoreList.Add(SelectedObject);
                }

                TransformerPawn->MouseTraceByChannel(10000.f, ECC_Visibility, IgnoreList, false);   // Å© ãÛîzóÒÇ≈ÇÕÇ»Ç≠ IgnoreList ÇìnÇ∑
            }
            return;
        }

        if (SelectedObject)
        {
            SelectedObject = nullptr;

            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->DeselectAll();
            }
            return;
        }

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
