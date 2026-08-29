// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/Widget.h"
#include "PlayModeControl.generated.h"

/**
 *
 */
	UCLASS(Blueprintable)
	class SKG_PROJECT_API UPlayModeControl : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Capture")
	bool bScreenshotEnabled = false;

	UPROPERTY(BlueprintReadWrite, Category = "Capture")
	bool bRecordingEnabled = false;

	UPROPERTY(BlueprintReadWrite, Category = "Capture")
	UWidget* CaptureTargetWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Capture")
	float CaptureInterval = 1.0f;





	UFUNCTION(BlueprintCallable, Category = "PlayMode")
	void StartPlayMode();

	UFUNCTION(BlueprintCallable, Category = "PlayMode")
	void StopPlayMode();

private:
	void OnCaptureTick();

	FTimerHandle CaptureTimer;
};
