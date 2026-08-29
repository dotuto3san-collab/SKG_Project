// Fill out your copyright notice in the Description page of Project Settings.


#include "ObjectHUDWidget.h"
#include "Components/TextBlock.h"

void UObjectHUDWidget::SetCurrentObjectText(const FText& NewText)
{
    if (CurrentObjectText)
    {
        CurrentObjectText->SetText(NewText);
    }
}