// Fill out your copyright notice in the Description page of Project Settings.


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
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &AObjectPlacer::OnLeftClickPressed);
    InputComponent->BindAction("LeftClick", IE_Released, this, &AObjectPlacer::OnLeftClickReleased);
}

void AObjectPlacer::OnLeftClickPressed()
{
    bIsLeftMouseDown = true;

    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        AActor* HitActor = Hit.GetActor();

        // すでに置いてあるオブジェクトをクリックした場合 → 選択
        if (HitActor && PlacedObjects.Contains(HitActor))
        {
            if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
            {
                TransformerPawn->SelectActor(HitActor);
            }
            return; // ここで処理終了、新規配置はしない
        }
        // それ以外(床など何もない場所)をクリックした場合 → 新規配置
        FVector PlacementLocation = Hit.Location + FVector(0.f, 0.f, 50.f);
        AActor* NewObject = GetWorld()->SpawnActor<AActor>(ObjectToSpawn, PlacementLocation, FRotator::ZeroRotator);

        if (NewObject)
        {
            PlacedObjects.Add(NewObject);
            SetObjectColor(NewObject, FLinearColor::White);

            if (GEngine)
            {
                FString Msg = FString::Printf(TEXT("Count: %d"), PlacedObjects.Num());
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
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
}

void AObjectPlacer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsLeftMouseDown)
    {
        if (ATransformerPawn* TransformerPawn = Cast<ATransformerPawn>(GetPawn()))
        {
            TransformerPawn->MouseTraceByChannel(10000.f, ECC_Visibility, TArray<AActor*>(), false);
        }
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
