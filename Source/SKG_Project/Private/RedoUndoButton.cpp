// Fill out your copyright notice in the Description page of Project Settings.

#include "RedoUndoButton.h"

URedoUndoButton::URedoUndoButton()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// --- RedoUndoButton.cpp ---

void URedoUndoButton::RecordTransformCommand(AActor* TargetActor, FTransform OldTransform, FTransform NewTransform)
{
	if (!TargetActor) return;
	TSharedPtr<ICommand> Cmd = MakeShared<FTransformCommand>(TargetActor, OldTransform, NewTransform);
	UndoStack.Push(Cmd);
	RedoStack.Empty();
	UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] Transform Recorded: %s (Stack Size: %d)"), *TargetActor->GetName(), UndoStack.Num());
}

void URedoUndoButton::RecordSpawnCommand(AActor* SpawnedActor)
{
	if (!SpawnedActor) return;
	TSharedPtr<ICommand> Cmd = MakeShared<FSpawnCommand>(SpawnedActor);
	UndoStack.Push(Cmd);
	RedoStack.Empty();
	UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] Spawn Recorded: %s (Stack Size: %d)"), *SpawnedActor->GetName(), UndoStack.Num());
}

void URedoUndoButton::RecordDestroyCommand(AActor* TargetActor)
{
	if (!TargetActor) return;
	TSharedPtr<ICommand> Cmd = MakeShared<FDestroyCommand>(TargetActor);
	Cmd->Execute(); // ”ñ•\Ž¦‰»
	UndoStack.Push(Cmd);
	RedoStack.Empty();
	UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] Destroy Recorded: %s (Stack Size: %d)"), *TargetActor->GetName(), UndoStack.Num());
}

bool URedoUndoButton::PerformUndo()
{
	if (UndoStack.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] PerformUndo Failed: Stack is Empty!"));
		return false;
	}

	TSharedPtr<ICommand> Cmd = UndoStack.Pop();
	Cmd->Undo();
	RedoStack.Push(Cmd);
	UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] PerformUndo Success! Remaining Stack: %d"), UndoStack.Num());
	return true;
}

bool URedoUndoButton::PerformRedo()
{
	if (RedoStack.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] PerformRedo Failed: RedoStack is Empty!"));
		return false;
	}

	TSharedPtr<ICommand> Cmd = RedoStack.Pop();
	Cmd->Execute();
	UndoStack.Push(Cmd);
	UE_LOG(LogTemp, Warning, TEXT("[UndoSystem] PerformRedo Success! Remaining Stack: %d"), RedoStack.Num());
	return true;
}
