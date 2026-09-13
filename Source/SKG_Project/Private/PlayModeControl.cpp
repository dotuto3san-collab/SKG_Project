// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayModeControl.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/PlatformFileManager.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#endif

void UPlayModeControl::StartPlayMode()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().SetTimer(
			CaptureTimer,
			this,
			&UPlayModeControl::OnCaptureTick,
			ScreenshotInterval,
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

void UPlayModeControl::Tick(float DeltaTime)
{
	if (bRecordingEnabled && !bWasRecordingLastTick)
	{
		SegmentIndex = 0;
		SegmentElapsedTime = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("Recording started. Segment %d"), SegmentIndex);
		StartNewVideoSegment();
	}

	if (!bRecordingEnabled && bWasRecordingLastTick)
	{
		UE_LOG(LogTemp, Warning, TEXT("Recording stopped."));
		FinalizeCurrentVideoSegment();
	}

	if (bRecordingEnabled)
	{

		if (!GEngine || !GEngine->GameViewport)
		{
			bWasRecordingLastTick = bRecordingEnabled;
			return;
		}

		FViewport* Viewport = GEngine->GameViewport->Viewport;

		if (!Viewport)
		{
			bWasRecordingLastTick = bRecordingEnabled;
			return;
		}

		TArray<FColor> Bitmap;
		Viewport->ReadPixels(Bitmap);

		UE_LOG(LogTemp, Log, TEXT("Captured 1 frame: %d pixels, DeltaTime %.4f"), Bitmap.Num(), DeltaTime);

		SegmentElapsedTime += DeltaTime;
		if (SegmentElapsedTime >= RecordingSegmentSeconds)
		{
			SegmentIndex++;
			UE_LOG(LogTemp, Warning, TEXT("Segment boundary reached (%.1fs). Moving to segment %d"), RecordingSegmentSeconds, SegmentIndex);
			SegmentElapsedTime = 0.0f;
			StartNewVideoSegment();
		}
	}
	bWasRecordingLastTick = bRecordingEnabled;
}

void UPlayModeControl::BeginDestroy()
{
	FinalizeCurrentVideoSegment();
#if PLATFORM_WINDOWS
	if (bMFInitialized)
	{
		MFShutdown();
		bMFInitialized = false;
	}
#endif
	Super::BeginDestroy();
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

void UPlayModeControl::StartNewVideoSegment()
{
	FinalizeCurrentVideoSegment();

#if PLATFORM_WINDOWS
	if (!bMFInitialized)
	{
		HRESULT hr = MFStartup(MF_VERSION);
		bMFInitialized = SUCCEEDED(hr);
		if (!bMFInitialized)
		{
			UE_LOG(LogTemp, Error, TEXT("MFStartup failed: 0x%08x"), hr);
			return;
		}
	}

	const FString FilePath = GetSegmentFilePath(SegmentIndex);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

	IMFAttributes* Attributes = nullptr;
	MFCreateAttributes(&Attributes, 1);

	Attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);

	HRESULT hr = MFCreateSinkWriterFromURL(*FilePath, nullptr, Attributes, &SinkWriter);

	if (Attributes) { Attributes->Release(); Attributes = nullptr; }

	if (FAILED(hr) || !SinkWriter)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create SinkWriter for %s (0x%08x)"), *FilePath, hr);
		SinkWriter = nullptr;
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Video segment file created: %s"), *FilePath);

#endif
}

void UPlayModeControl::FinalizeCurrentVideoSegment()
{
#if PLATFORM_WINDOWS
	if (SinkWriter)
	{
		SinkWriter->Finalize();
		SinkWriter->Release();
		SinkWriter = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Video segment finalized."));
	}
#endif
}

FString UPlayModeControl::GetSegmentFilePath(int32 Index) const
{
	return FPaths::ProjectSavedDir()
		+ TEXT("Recordings/Segment_")
		+ FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S_"))
		+ FString::FromInt(Index)
		+ TEXT(".mp4");
}

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif