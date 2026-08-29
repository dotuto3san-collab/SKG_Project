// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayModeControl.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

void UPlayModeControl::StartPlayMode()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().SetTimer(
			CaptureTimer,
			this,
			&UPlayModeControl::OnCaptureTick,
			CaptureInterval,
			true
		);
	}
}

void UPlayModeControl::StopPlayMode()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().ClearTimer(CaptureTimer);
	}
}



void UPlayModeControl::OnCaptureTick()
{
	UE_LOG(LogTemp, Warning, TEXT("OnCaptureTick called! bScreenshotEnabled = %d"), bScreenshotEnabled);

	if (bScreenshotEnabled)
	{
		FString Filename = FPaths::ProjectSavedDir()
			+ TEXT("Screenshots/Capture_")
			+ FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
			+ TEXT(".png");

		FScreenshotRequest::RequestScreenshot(Filename, false, false);
	}

	if (bRecordingEnabled)
	{

	}
}
