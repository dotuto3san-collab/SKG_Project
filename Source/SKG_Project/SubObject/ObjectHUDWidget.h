// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ObjectHUDWidget.generated.h"

class UTextBlock;

/**
 *
 */
UCLASS()
class SKG_PROJECT_API UObjectHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetCurrentObjectText(const FText& NewText);

protected:
    // WBP_ObjectHUD側のテキストブロック変数名(CurrentObjectText)と完全一致させること
    UPROPERTY(meta = (BindWidget))
    UTextBlock* CurrentObjectText;
};
