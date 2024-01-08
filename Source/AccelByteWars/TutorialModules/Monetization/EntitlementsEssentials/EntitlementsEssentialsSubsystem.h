﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "EntitlementsEssentialsModel.h"
#include "Core/AssetManager/TutorialModules/TutorialModuleSubsystem.h"
#include "OnlineEntitlementsInterfaceAccelByte.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "EntitlementsEssentialsSubsystem.generated.h"

UCLASS()
class ACCELBYTEWARS_API UEntitlementsEssentialsSubsystem : public UTutorialModuleSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override {return true;}

public:
	UItemDataObject* GetItemEntitlement(const FUniqueNetIdPtr UserId, const FUniqueOfferId ItemId) const;

	void QueryUserEntitlement(const FUniqueNetIdPtr UserId);
	FOnQueryUserEntitlementsComplete OnQueryUserEntitlementsCompleteDelegates;

	bool GetIsQueryRunning() const { return bIsQueryRunning; }

	void ConsumeItemEntitlement(
		const FUniqueNetIdPtr UserId,
		const FString& ItemId,
		const int32 UseCount, 
		const FOnConsumeUserEntitlementComplete& OnComplete = FOnConsumeUserEntitlementComplete());

private:
	void OnQueryEntitlementComplete(
		bool bWasSuccessful,
		const FUniqueNetId& UserId,
		const FString& Namespace,
		const FString& Error);

	void OnConsumeEntitlementComplete(
		bool bWasSuccessful, 
		const FUniqueNetId& UserId, 
		const TSharedPtr<FOnlineEntitlement>& Entitlement, 
		const FOnlineError& Error,
		const FOnConsumeUserEntitlementComplete OnComplete);

	void OnQueryToConsumeEntitlementComplete(
		bool bWasSuccessful, 
		const FUniqueNetId& UserId, 
		const FString& Namespace, 
		const FString& Error,
		const FString ItemId, 
		const int32 UseCount, 
		const FOnConsumeUserEntitlementComplete OnComplete);

	FUniqueNetIdPtr GetLocalPlayerUniqueNetId(const APlayerController* PlayerController) const;

	FOnlineEntitlementsAccelBytePtr EntitlementsInterface;

	bool bIsQueryRunning = false;

	FDelegateHandle ConsumeEntitlementDelegateHandle;
	FDelegateHandle QueryToConsumeEntitlementDelegateHandle;
};
