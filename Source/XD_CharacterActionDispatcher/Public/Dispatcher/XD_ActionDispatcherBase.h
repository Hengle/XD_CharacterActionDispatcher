﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_CharacterActionDispatcherType.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "XD_ActionDispatcherBase.generated.h"

class UXD_DispatchableActionBase;
class UXD_ActionDispatcherManager;

/**
 * 
 */
USTRUCT()
struct FTogetherFlowControl
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	TArray<bool> CheckList;

	UPROPERTY(SaveGame)
	FOnDispatchableActionFinishedEvent TogetherEvent;
};

DECLARE_DELEGATE_OneParam(FWhenDispatchFinishedNative, const FName& /*Tag*/);
DECLARE_DELEGATE(FOnDispatcherAbortedNative);
DECLARE_DELEGATE_OneParam(FOnDispatchDeactiveNative, bool /*IsFinsihedCompleted*/);

UCLASS(abstract, BlueprintType)
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherBase : public UObject
{
	GENERATED_BODY()
public:
	UXD_ActionDispatcherBase();

	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanStartDispatcher() const;
	UFUNCTION(BlueprintNativeEvent, Category = "行为", meta = (DisplayName = "CanStartDispatcher"))
	bool ReceiveCanStartDispatcher() const;

	bool IsDispatcherValid() const;
	//用于确保调度器里调度的所有对象有效性
	UFUNCTION(BlueprintNativeEvent, Category = "行为", meta = (DisplayName = "IsDispatcherValid"))
	bool ReceiveIsDispatcherValid() const;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void StartDispatch();
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void InitLeader(AActor* InDispatcherLeader);

	void StartDispatchWithEvent(const FOnDispatchDeactiveNative& OnDispatchDeactive);

	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "执行调度"))
	void WhenDispatchStart();

	bool IsDispatcherStarted() const { return CurrentActions.Num() > 0; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void InvokeActiveAction(UXD_DispatchableActionBase* Action);

public:
	FOnDispatcherAborted OnDispatcherAborted;
	FOnDispatcherAbortedNative OnDispatcherAbortedNative;

	void AbortDispatch(UXD_DispatchableActionBase* DeactiveRequestAction = nullptr);
	void AbortDispatch(const FOnDispatcherAborted& Event, UXD_DispatchableActionBase* DeactiveRequestAction = nullptr);
	void AbortDispatch(const FOnDispatcherAbortedNative& Event, UXD_DispatchableActionBase* DeactiveRequestAction = nullptr);

	UFUNCTION(BlueprintCallable, Category = "行为", meta = (DisplayName = "AbortDispatch"))
	void BP_AbortDispatch(const FOnDispatcherAborted& Event);

	UFUNCTION(BlueprintCallable, Category = "行为")
	void AssignOnDispatcherAbort(const FOnDispatcherAborted& Event);
protected:
	void ExecuteAbortedDelegate();
	void WhenActionAborted();

	void DeactiveDispatcher(bool IsFinsihedCompleted);
	void SaveDispatchState();

	bool CanReactiveDispatcher() const;

protected:
	TArray<USoftObjectProperty*> SoftObjectPropertys;
	const TArray<USoftObjectProperty*>& GetSoftObjectPropertys() const;
	void PostCDOContruct() override;

public:
	//调度器的主导者，为所在关卡或者玩家的角色
	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> DispatcherLeader;

	UFUNCTION()
	void WhenPlayerLeaderDestroyed(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	void WhenLevelLeaderDestroyed(ULevel* Level);

protected:
	friend class UXD_ActionDispatcherManager;
	friend class UXD_DispatchableActionBase;
	bool InvokeReactiveDispatch();
	void ReactiveDispatcher();

	void ActiveDispatcher();
private:
	bool IsAllSoftReferenceValid() const;
	//结束调度器
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void FinishDispatch(FGameplayTag Tag);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDispatchFinished, const FGameplayTag&, Tag);
	UPROPERTY(BlueprintAssignable)
	FOnDispatchFinished OnDispatchFinished;

	DECLARE_DYNAMIC_DELEGATE_OneParam(FWhenDispatchFinished, const FName&, Tag);
	UPROPERTY(SaveGame)
	FWhenDispatchFinished WhenDispatchFinished;

	FWhenDispatchFinishedNative WhenDispatchFinishedNative;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void BindWhenDispatchFinished(const FWhenDispatchFinished& DispatchFinishedEvent) { WhenDispatchFinished = DispatchFinishedEvent; }

#if WITH_EDITORONLY_DATA
	TArray<FName> GetAllFinishTags() const;
#endif
public:
	UPROPERTY(SaveGame)
	TArray<UXD_DispatchableActionBase*> CurrentActions;

	UPROPERTY(BlueprintReadOnly, Category = "行为调度器")
	EActionDispatcherState State;

	// 调度器可能不存在管理器
	// e.g. 玩家开机关的行为，调度器直接交给机关管理
	UXD_ActionDispatcherManager* GetManager() const;

	UWorld* GetWorld() const override;
	
	//共同行为调度器
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	bool EnterTogetherFlowControl(FGuid NodeGuid, int32 Index, int32 TogetherCount);

	TMap<FGuid, FTogetherFlowControl> ActivedTogetherControl;

	//子流程，允许逻辑分层
	//若没有需要储存的状态更推荐使用公共宏代替
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	bool IsSubActionDispatcher() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = true))
	UXD_ActionDispatcherBase* GetMainActionDispatcher();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void ActiveSubActionDispatcher(UXD_ActionDispatcherBase* SubActionDispatcher, FGuid NodeGuid);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	bool TryActiveSubActionDispatcher(FGuid NodeGuid);

	UPROPERTY(SaveGame)
	TMap<FGuid, UXD_ActionDispatcherBase*> ActivedSubActionDispatchers;

public:
	virtual void WhenActived() { ReceiveWhenActived(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "交互", meta = (DisplayName = "WhenActived"))
	void ReceiveWhenActived();

	FOnDispatchDeactiveNative OnDispatchDeactiveNative;
	virtual void WhenDeactived(bool IsFinsihedCompleted);
	UFUNCTION(BlueprintImplementableEvent, Category = "交互", meta = (DisplayName = "WhenDeactived"))
	void ReceiveWhenDeactived(bool IsFinsihedCompleted);
};
