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

#include "SceneSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SKG_Project/SubObject/ObjectPlacer.h" 

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
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: World is null"));
        return;
    }
    if (PlaceableBaseClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: PlaceableBaseClasses is not set"));
        return;
    }

    // Ask which file to open
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        return;
    }

    TArray<FString> OutFiles;
    const bool bOpened = DesktopPlatform->OpenFileDialog(
        nullptr,
        TEXT("Open Scene"),
        TEXT(""),
        TEXT(""),
        TEXT("Scene Files (*.scene)|*.scene"),
        EFileDialogFlags::None,
        OutFiles
    );

    if (!bOpened || OutFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: canceled"));
        return;
    }

    // Read the file, then turn the bytes back into save data
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *OutFiles[0]))
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: failed to read file: %s"), *OutFiles[0]);
        return;
    }

    USceneSaveGame* LoadData = Cast<USceneSaveGame>(
        UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!LoadData)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: file is not a valid scene file: %s"), *OutFiles[0]);
        return;
    }

    // Before removing the old actors, clear selection, gizmo, lock and undo history
    AObjectPlacer* Placer = Cast<AObjectPlacer>(UGameplayStatics::GetPlayerController(World, 0));
    if (Placer)
    {
        Placer->ResetForSceneLoad();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: AObjectPlacer not found"));
    }

    // 1) Remove the objects that are placed now (skip hidden ones)
    TArray<AActor*> ToDestroy;
    for (const TSubclassOf<AActor>& BaseClass : PlaceableBaseClasses)
    {
        if (!BaseClass)
        {
            continue;
        }

        for (TActorIterator<AActor> It(World, BaseClass); It; ++It)
        {
            AActor* Actor = *It;
            if (IsValid(Actor) && !Actor->IsHidden())
            {
                ToDestroy.AddUnique(Actor);
            }
        }
    }

    for (AActor* Actor : ToDestroy)
    {
        Actor->Destroy();
    }

    // 2) Spawn the saved objects again
    int32 SpawnedCount = 0;
    for (const FSavedObjectData& Data : LoadData->Objects)
    {
        UClass* ObjClass = StaticLoadClass(AActor::StaticClass(), nullptr, *Data.ClassPath);
        if (!ObjClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: class not found: %s"), *Data.ClassPath);
            continue;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* NewActor = World->SpawnActor<AActor>(ObjClass, Data.Transform, Params);
        if (NewActor)
        {
            ++SpawnedCount;

            if (Placer)
            {
                Placer->RegisterLoadedActor(NewActor);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: destroyed=%d spawned=%d / %d file=%s"),
        ToDestroy.Num(), SpawnedCount, LoadData->Objects.Num(), *OutFiles[0]);
}

void URibbonControl::SaveSceneFile()
{
    UWorld* World = GetWorld();
    if (!World || PlaceableBaseClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: World or PlaceableBaseClasses is not set"));
        return;
    }

    // Ask where to save
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        return;
    }

    TArray<FString> OutFiles;
    const bool bChosen = DesktopPlatform->SaveFileDialog(
        nullptr,
        TEXT("Save Scene"),
        TEXT(""),
        TEXT("Scene01.scene"),
        TEXT("Scene Files (*.scene)|*.scene"),
        EFileDialogFlags::None,
        OutFiles
    );

    if (!bChosen || OutFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: canceled"));
        return;
    }

    FString FilePath = OutFiles[0];
    if (FPaths::GetExtension(FilePath).IsEmpty())
    {
        FilePath += TEXT(".scene");
    }

    USceneSaveGame* SaveData = Cast<USceneSaveGame>(
        UGameplayStatics::CreateSaveGameObject(USceneSaveGame::StaticClass()));

    // Remember actors already saved, so none is saved twice
    TSet<AActor*> Saved;

    for (const TSubclassOf<AActor>& BaseClass : PlaceableBaseClasses)
    {
        if (!BaseClass)
        {
            continue;
        }

        for (TActorIterator<AActor> It(World, BaseClass); It; ++It)
        {
            AActor* Actor = *It;
            // Skip deleted actors (the undo system may only hide them)
            if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->IsHidden())
            {
                UE_LOG(LogTemp, Warning, TEXT("Skipped (deleted or hidden): %s"), *Actor->GetName());
                continue;
            }
            if (Saved.Contains(Actor))
            {
                continue;
            }
            Saved.Add(Actor);

            FSavedObjectData Data;
            Data.ClassPath = Actor->GetClass()->GetPathName();
            Data.Transform = Actor->GetActorTransform();
            SaveData->Objects.Add(Data);
        }
    }

    // Turn the save data into bytes, then write them to the file
    TArray<uint8> Bytes;
    if (!UGameplayStatics::SaveGameToMemory(SaveData, Bytes))
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: failed to make save data"));
        return;
    }

    const bool bSaved = FFileHelper::SaveArrayToFile(Bytes, *FilePath);
    UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: objects=%d saved=%d file=%s"),
        SaveData->Objects.Num(), bSaved ? 1 : 0, *FilePath);
}

void URibbonControl::Recents()
{
}