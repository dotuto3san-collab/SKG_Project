// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RibbonControl.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class SKG_PROJECT_API URibbonControl : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "File")
	void ImportPointCloud();

	UFUNCTION(BlueprintCallable, Category = "File")
	void LoadSceneFile();

	UFUNCTION(BlueprintCallable, Category = "File")
	void SaveSceneFile();

	UFUNCTION(BlueprintCallable, Category = "File")
	void Recents();
};
