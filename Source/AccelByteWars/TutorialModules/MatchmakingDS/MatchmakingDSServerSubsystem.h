﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "MatchmakingDSOnlineSession.h"
#include "TutorialModules/GameSessionEssentials/AccelByteWarsServerSubsystemBase.h"
#include "MatchmakingDSServerSubsystem.generated.h"

UCLASS()
class ACCELBYTEWARS_API UMatchmakingDSServerSubsystem : public UAccelByteWarsServerSubsystemBase
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TSubclassOf<UTutorialModuleOnlineSession> GetOnlineSessionClass() const override
	{
		return UMatchmakingDSOnlineSession::StaticClass();
	}

	virtual void RegisterServer(const FName SessionName) override;
	virtual void UnregisterServer(const FName SessionName) override;

protected:
	virtual void OnRegisterServerComplete(bool bSucceeded) override;
	virtual void OnUnregisterServerComplete(bool bSucceeded) override;

	virtual void OnServerSessionReceived(FName SessionName) override;

private:
	bool bServerAlreadyRegister = false;
	bool bUnregisterServerRunning = false;

	UPROPERTY()
	UMatchmakingDSOnlineSession* MatchmakingOnlineSession;
};
