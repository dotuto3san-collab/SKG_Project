// Fill out your copyright notice in the Description page of Project Settings.


#include "ObjectPlacer.h"

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

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Selected!"));
            }
            return; // ここで処理終了、新規配置はしない
        }
        // それ以外(床など何もない場所)をクリックした場合 → 新規配置
        FVector PlacementLocation = Hit.Location + FVector(0.f, 0.f, 50.f);
        AActor* NewObject = GetWorld()->SpawnActor<AActor>(ObjectToSpawn, PlacementLocation, FRotator::ZeroRotator);

        if (NewObject)
        {
            PlacedObjects.Add(NewObject);

            if (GEngine)
            {
                FString Msg = FString::Printf(TEXT("Count: %d"), PlacedObjects.Num());
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
            }
        }
    }
}