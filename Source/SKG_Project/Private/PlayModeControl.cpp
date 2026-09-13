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
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		UE_LOG(LogTemp, Error, TEXT("No viewport available, cannot start recording."));
		return;
	}

	const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	RecordingWidth = Size.X;
	RecordingHeight = Size.Y;

	RecordingWidth -= (RecordingWidth % 2);
	RecordingHeight -= (RecordingHeight % 2);

	const int32 TargetFps = 30;
	RecordingFrameDuration100ns = 10 * 1000 * 1000 / TargetFps;
	RecordingFrameCount = 0;

	const FString FilePath = GetSegmentFilePath(SegmentIndex);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

	IMFAttributes* WriterAttributes = nullptr;
	MFCreateAttributes(&WriterAttributes, 1);

	WriterAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);

	HRESULT hr = MFCreateSinkWriterFromURL(*FilePath, nullptr, WriterAttributes, &SinkWriter);

	if (WriterAttributes) { WriterAttributes->Release(); WriterAttributes = nullptr; }

	if (FAILED(hr) || !SinkWriter)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create SinkWriter for %s (0x%08x)"), *FilePath, hr);
		SinkWriter = nullptr;
		return;
	}


	// OutputFormat
	IMFMediaType* OutputType = nullptr;
	MFCreateMediaType(&OutputType);
	OutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	OutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	OutputType->SetUINT32(MF_MT_AVG_BITRATE, 8000000); // 8Mbps
	OutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(OutputType, MF_MT_FRAME_SIZE, RecordingWidth, RecordingHeight);
	MFSetAttributeRatio(OutputType, MF_MT_FRAME_RATE, TargetFps, 1);
	MFSetAttributeRatio(OutputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	hr = SinkWriter->AddStream(OutputType, &VideoStreamIndex);
	OutputType->Release();

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("AddStream failed: 0x%08x"), hr);
		FinalizeCurrentVideoSegment();
		return;
	}


	// InputFormat
	IMFMediaType* InputType = nullptr;
	MFCreateMediaType(&InputType);
	InputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	InputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	InputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(InputType, MF_MT_FRAME_SIZE, RecordingWidth, RecordingHeight);
	MFSetAttributeRatio(InputType, MF_MT_FRAME_RATE, TargetFps, 1);
	MFSetAttributeRatio(InputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	hr = SinkWriter->SetInputMediaType(VideoStreamIndex, InputType, nullptr);
	InputType->Release();

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("SetInputMediaType failed: 0x%08x"), hr);
		FinalizeCurrentVideoSegment();
		return;
	}

	hr = SinkWriter->BeginWriting();
	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("BeginWriting failed: 0x%08x"), hr);
		FinalizeCurrentVideoSegment();
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