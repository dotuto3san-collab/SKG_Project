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
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("Scene01"), 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: save data not found"));
        return;
    }

    USceneSaveGame* LoadData = Cast<USceneSaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("Scene01"), 0));
    if (!LoadData)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: failed to load save data"));
        return;
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
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("LoadSceneFile: destroyed=%d spawned=%d / %d"),
        ToDestroy.Num(), SpawnedCount, LoadData->Objects.Num());
}

void URibbonControl::SaveSceneFile()
{
    UWorld* World = GetWorld();
    if (!World || PlaceableBaseClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: World or PlaceableBaseClasses is not set"));
        return;
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

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveData, TEXT("Scene01"), 0);
    UE_LOG(LogTemp, Warning, TEXT("SaveSceneFile: objects=%d saved=%d"),
        SaveData->Objects.Num(), bSaved ? 1 : 0);
}

void URibbonControl::Recents()
{
}