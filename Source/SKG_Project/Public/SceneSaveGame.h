// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SceneSaveGame.generated.h"

// One placed object
USTRUCT()
struct FSavedObjectData
{
	GENERATED_BODY()

	// Which class to spawn when loading
	UPROPERTY()
	FString ClassPath;

	UPROPERTY()
	FTransform Transform;
};

// One point cloud (only the file path, not the points)
USTRUCT()
struct FSavedPointCloudData
{
	GENERATED_BODY()

	UPROPERTY()
	FString FilePath;

	UPROPERTY()
	FTransform Transform;
};

UCLASS()
class SKG_PROJECT_API USceneSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSavedObjectData> Objects;

	UPROPERTY()
	TArray<FSavedPointCloudData> PointClouds;
};