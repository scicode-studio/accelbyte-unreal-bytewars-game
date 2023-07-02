// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"
#include "AccelByteWarsPlayerController.generated.h"

UCLASS()
class ACCELBYTEWARS_API AAccelByteWarsPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Trigger to start the game from game lobby.
	UFUNCTION(Reliable, Server, meta = (WorldContext = "WorldContextObject"))
	void TriggerLobbyStart();

	TSharedRef<FOnlineSessionSearch> SessionSearch = MakeShared<FOnlineSessionSearch>(FOnlineSessionSearch());

private:
	void LoadingPlayerAssignment() const;
};