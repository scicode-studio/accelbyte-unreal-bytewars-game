// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "MatchmakingDSOnlineSession_Starter.h"
#include "TutorialModules/GameSessionEssentials/AccelByteWarsServerSubsystemBase.h"
#include "MatchmakingDSServerSubsystem_Starter.generated.h"

UCLASS()
class ACCELBYTEWARS_API UMatchmakingDSServerSubsystem_Starter : public UAccelByteWarsServerSubsystemBase
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TSubclassOf<UTutorialModuleOnlineSession> GetOnlineSessionClass() const override
	{
		return UMatchmakingDSOnlineSession_Starter::StaticClass();
	}

#pragma region "Matchmaking with DS Function Declarations"
protected:
	// TODO: Add your module protected function declarations here.
#pragma endregion

private:
	UPROPERTY()
	UMatchmakingDSOnlineSession_Starter* MatchmakingOnlineSession;
};
