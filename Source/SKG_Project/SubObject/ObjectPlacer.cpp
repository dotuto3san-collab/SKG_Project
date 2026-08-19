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
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("クリックされました！"));
    }
}