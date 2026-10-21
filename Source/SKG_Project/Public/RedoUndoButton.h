// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RedoUndoButton.generated.h"

// 操作の基底インターフェース
class ICommand
{
public:
	virtual ~ICommand() {}
	virtual void Execute() = 0; // 実行 / Redo
	virtual void Undo() = 0;    // 取り消し
};

// 1. 移動・回転・拡大（Transform変更）のコマンド
class FTransformCommand : public ICommand
{
public:
	FTransformCommand(AActor* InActor, const FTransform& InOldTransform, const FTransform& InNewTransform)
		: TargetActor(InActor), OldTransform(InOldTransform), NewTransform(InNewTransform) {
	}

	virtual void Execute() override
	{
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorTransform(NewTransform);
		}
	}

	virtual void Undo() override
	{
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorTransform(OldTransform);
		}
	}

private:
	TWeakObjectPtr<AActor> TargetActor;
	FTransform OldTransform;
	FTransform NewTransform;
};

// 2. 設置（Spawn）のコマンド
class FSpawnCommand : public ICommand
{
public:
	FSpawnCommand(AActor* InSpawnedActor)
		: TargetActor(InSpawnedActor)
	{
		if (InSpawnedActor)
		{
			ActorClass = InSpawnedActor->GetClass();
			SavedTransform = InSpawnedActor->GetActorTransform();
		}
	}

	virtual void Execute() override
	{
		// Redo: 非表示/コリジョンOFFから復元（または再生成）
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorHiddenInGame(false);
			TargetActor->SetActorEnableCollision(true);
		}
	}

	virtual void Undo() override
	{
		// Undo: 設置を取り消す（非表示 ＆ コリジョンOFF）
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorHiddenInGame(true);
			TargetActor->SetActorEnableCollision(false);
		}
	}

private:
	TWeakObjectPtr<AActor> TargetActor;
	UClass* ActorClass;
	FTransform SavedTransform;
};

// 3. 削除（Destroy）のコマンド
class FDestroyCommand : public ICommand
{
public:
	FDestroyCommand(AActor* InDestroyedActor)
		: TargetActor(InDestroyedActor) {
	}

	virtual void Execute() override
	{
		// Redo: 再度削除（非表示 ＆ コリジョンOFF）
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorHiddenInGame(true);
			TargetActor->SetActorEnableCollision(false);
		}
	}

	virtual void Undo() override
	{
		// Undo: 削除を取り消して復元
		if (TargetActor.IsValid())
		{
			TargetActor->SetActorHiddenInGame(false);
			TargetActor->SetActorEnableCollision(true);
		}
	}

private:
	TWeakObjectPtr<AActor> TargetActor;
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SKG_PROJECT_API URedoUndoButton : public UActorComponent
{
	GENERATED_BODY()

public:
	URedoUndoButton();

	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	bool PerformUndo();

	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	bool PerformRedo();

	// 移動・回転・拡大の記録
	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	void RecordTransformCommand(AActor* TargetActor, FTransform OldTransform, FTransform NewTransform);

	// 設置の記録
	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	void RecordSpawnCommand(AActor* SpawnedActor);

	// 削除の記録
	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	void RecordDestroyCommand(AActor* TargetActor);

	// Undo/Redoの履歴を全部消す(シーン読み込み時に使う)
	UFUNCTION(BlueprintCallable, Category = "UndoRedo")
	void ClearHistory();

private:
	TArray<TSharedPtr<ICommand>> UndoStack;
	TArray<TSharedPtr<ICommand>> RedoStack;
};