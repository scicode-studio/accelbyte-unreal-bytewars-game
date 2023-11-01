﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "MatchSessionP2POnlineSession.h"
#include "Play/GameSessionEssentials/AccelByteWarsServerSubsystemBase.h"
#include "MatchSessionP2PServerSubsystem.generated.h"

UCLASS()
class ACCELBYTEWARS_API UMatchSessionP2PServerSubsystem : public UAccelByteWarsServerSubsystemBase
{
	GENERATED_BODY()

#pragma region "Game specific"
protected:
	virtual void OnAuthenticatePlayerComplete_PrePlayerSetup(APlayerController* PlayerController) override;
#pragma endregion 

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	virtual void OnServerSessionReceived(FName SessionName) override;

private:
	UPROPERTY()
	UMatchSessionP2POnlineSession* OnlineSession;
};
