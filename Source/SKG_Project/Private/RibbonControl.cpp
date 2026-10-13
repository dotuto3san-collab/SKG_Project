// Fill out your copyright notice in the Description page of Project Settings.


#include "RibbonControl.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"

#include "LidarPointCloud.h"
#include "LidarPointCloudActor.h"
#include "LidarPointCloudComponent.h"
#include "Engine/World.h"
#include "Async/Async.h"


void URibbonControl::ImportPointCloud()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

    if (DesktopPlatform)
    {
        TArray<FString> OutFiles;

        bool bOpened = DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("Select Point Cloud Data"),
            TEXT(""),
            TEXT(""),
            TEXT("Point Cloud Files (*.las;*.laz;*.xyz;*.pts;*.e57)|*.las;*.laz;*.xyz;*.pts;*.e57|All Files (*.*)|*.*"),
            EFileDialogFlags::None,
            OutFiles
        );

        if (bOpened && OutFiles.Num() > 0)
        {
            UWorld* World = GetWorld();
            if (!World)
            {
                UE_LOG(LogTemp, Warning, TEXT("World is null"));
                return;
            }

            ULidarPointCloud* PointCloud = ULidarPointCloud::CreateFromFile(OutFiles[0]);
            if (!PointCloud)
            {
                UE_LOG(LogTemp, Warning, TEXT("Load Failed!: %s"), *OutFiles[0]);
                return;
            }

            ALidarPointCloudActor* Actor = World->SpawnActor<ALidarPointCloudActor>();
            Actor->SetPointCloud(PointCloud);

            UE_LOG(LogTemp, Warning, TEXT("Import started. Waiting for points..."));

            // Check every 0.5 seconds until points are loaded
            TWeakObjectPtr<ULidarPointCloud> WeakPC(PointCloud);
            TWeakObjectPtr<UWorld> WeakWorld(World);
            TSharedPtr<FTimerHandle> Handle = MakeShared<FTimerHandle>();

            World->GetTimerManager().SetTimer(
                *Handle,
                FTimerDelegate::CreateLambda([WeakPC, WeakWorld, Handle]()
                    {
                        UWorld* W = WeakWorld.Get();
                        ULidarPointCloud* PC = WeakPC.Get();
                        if (!W) { return; }

                        if (!PC)
                        {
                            W->GetTimerManager().ClearTimer(*Handle);
                            return;
                        }

                        if (PC->GetNumPoints() == 0)
                        {
                            return; // still loading
                        }

                        W->GetTimerManager().ClearTimer(*Handle);

                        FBox Bounds = PC->GetBounds();
                        FVector Size = Bounds.GetSize();
                        FVector Center = Bounds.GetCenter();
                        UE_LOG(LogTemp, Warning, TEXT("Points: %lld"), (long long)PC->GetNumPoints());
                        UE_LOG(LogTemp, Warning, TEXT("Size: X=%f Y=%f Z=%f"), Size.X, Size.Y, Size.Z);
                        UE_LOG(LogTemp, Warning, TEXT("Center: X=%f Y=%f Z=%f"), Center.X, Center.Y, Center.Z);
                    }),
                0.5f,
                true
            );
        }
    }
}

void URibbonControl::LoadSceneFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		bool bOpened = DesktopPlatform->OpenFileDialog(
			nullptr,
			TEXT("Selected Scene"),
			TEXT(""),
			TEXT(""),
			TEXT("Scene Files (*.json;*.scene)|*.json;*.scene|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (bOpened && OutFiles.Num() > 0)
		{
			const FString SelectedFile = OutFiles[0];
			UE_LOG(LogTemp, Warning, TEXT("Loaded scene file: %s"), *SelectedFile);
		}
	}
}

void URibbonControl::SaveSceneFile()
{
}

void URibbonControl::Recents()
{
}