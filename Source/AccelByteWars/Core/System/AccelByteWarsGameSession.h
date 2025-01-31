// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "AccelByteWarsGameSession.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegisterServer, FName /*SessionName*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnregisterServer, FName /*SessionName*/);

UCLASS()
class ACCELBYTEWARS_API AAccelByteWarsGameSession : public AGameSession
{
	GENERATED_BODY()
	
public:
	virtual void RegisterServer() override;
	void UnregisterServer();

	/*virtual bool AtCapacity(bool bSpectator) override;*/

	static inline FOnRegisterServer OnRegisterServerDelegates;
	static inline FOnUnregisterServer OnUnregisterServerDelegates;
};