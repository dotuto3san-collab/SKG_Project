// Fill out your copyright notice in the Description page of Project Settings.


#include "ObjectPlacer.h"

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
        GetWorld()->SpawnActor<AActor>(ObjectToSpawn, Hit.Location, FRotator::ZeroRotator);
    }
}