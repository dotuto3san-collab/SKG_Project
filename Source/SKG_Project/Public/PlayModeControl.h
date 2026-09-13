// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/Widget.h"
#include "PlayModeControl.generated.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <mfobjects.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

struct IMFSinkWriter;

/**
 *
 */
	UCLASS(Blueprintable)
	class SKG_PROJECT_API UPlayModeControl : public UObject, public FTickableGameObject
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
	float ScreenshotInterval = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Capture")
	float RecordingSegmentSeconds = 5.0f;


	UFUNCTION(BlueprintCallable, Category = "PlayMode")
	void StartPlayMode();

	UFUNCTION(BlueprintCallable, Category = "PlayMode")
	void StopPlayMode();


	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bRecordingEnabled; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayModeControl, STATGROUP_Tickables);
	}

	virtual void BeginDestroy() override;

private:
	void OnCaptureTick();

	void StartNewVideoSegment();
	void FinalizeCurrentVideoSegment();
	FString GetSegmentFilePath(int32 Index) const;


	FTimerHandle CaptureTimer;

	bool bWasRecordingLastTick = false;
	float SegmentElapsedTime = 0.0f;
	int32 SegmentIndex = 0;

	bool bMFInitialized = false;

	int32 RecordingWidth = 0;
	int32 RecordingHeight = 0;
	int64 RecordingFrameDuration100ns = 0;
	int64 RecordingFrameCount = 0;

#if PLATFORM_WINDOWS
	IMFSinkWriter* SinkWriter = nullptr;
	DWORD VideoStreamIndex = 0;
#endif
};
