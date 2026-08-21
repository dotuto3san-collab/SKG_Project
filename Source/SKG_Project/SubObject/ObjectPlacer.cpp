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
            // 前に選択していたものがあれば、覚えておいた元の色に戻す
            if (SelectedObject)
            {
                SetObjectColor(SelectedObject, OriginalColor);
            }

            // 新しく選択するオブジェクトの「今の色」を覚えておく
            if (UStaticMeshComponent* Mesh = HitActor->FindComponentByClass<UStaticMeshComponent>())
            {
                UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
                if (DynMat)
                {
                    DynMat->GetVectorParameterValue(FName("Color"), OriginalColor);
                }
                else
                {
                    OriginalColor = FLinearColor::White; // まだ一度も選択されたことがなければデフォルト白
                }
            }

            SelectedObject = HitActor;
            SetObjectColor(SelectedObject, FLinearColor::Yellow);

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
            SetObjectColor(NewObject, FLinearColor::White);

            if (GEngine)
            {
                FString Msg = FString::Printf(TEXT("Count: %d"), PlacedObjects.Num());
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
            }
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
