// Fill out your copyright notice in the Description page of Project Settings.


#include "RibbonControl.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"

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
			TEXT("All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (bOpened && OutFiles.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Selected file: %s"), *OutFiles[0]);
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