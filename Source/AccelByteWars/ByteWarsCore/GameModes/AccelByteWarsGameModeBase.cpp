// Copyright Epic Games, Inc. All Rights Reserved.


#include "AccelByteWarsGameModeBase.h"

#include "AccelByteWarsGameStateBase.h"
#include "ByteWarsCore/Player/AccelByteWarsPlayerState.h"
#include "ByteWarsCore/System/AccelByteWarsGameInstance.h"
#include "ByteWarsCore/System/AccelByteWarsGlobals.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"

#define GAMEMODE_LOG(FormatString, ...) UE_LOG(LogByteWarsGameMode, Log, TEXT(FormatString), __VA_ARGS__);

void AAccelByteWarsGameModeBase::InitGameState()
{
	Super::InitGameState();

	ByteWarsGameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance());
	ByteWarsGameState = GetGameState<AAccelByteWarsGameStateBase>();

	if (!ensure(ByteWarsGameInstance)) return;
	if (!ensure(ByteWarsGameState)) return;
}

void AAccelByteWarsGameModeBase::BeginPlay()
{
	// Check if GameSetup have already been set up or not
	if (!ByteWarsGameInstance->GameSetup)
	{
		// have not yet set up, set GameSetup based on launch argument
		FString CodeName;
		FParse::Value(FCommandLine::Get(), TEXT("-GameMode="), CodeName);
		ByteWarsGameInstance->AssignGameMode(CodeName);
	}

	// Setup GameState variables if in GameplayLevel or if DedicatedServer
	if (bIsGameplayLevel || IsRunningDedicatedServer())
	{
		// GameState setup
		ByteWarsGameState->TimeLeft = ByteWarsGameInstance->GameSetup.MatchTime;

		// setup existing players
		for(FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			PlayerSetup(Iterator->Get());
		}
	}

	Super::BeginPlay();
}

APlayerController* AAccelByteWarsGameModeBase::Login(
	UPlayer* NewPlayer,
	ENetRole InRemoteRole,
	const FString& Portal,
	const FString& Options,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	APlayerController* PlayerController = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	// setup player if in GameplayLevel and game started
	if ((bIsGameplayLevel || IsRunningDedicatedServer()) && HasMatchStarted())
	{
		PlayerSetup(PlayerController);
	}

	return PlayerController;
}

void AAccelByteWarsGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (const AAccelByteWarsPlayerState* PlayerState = static_cast<AAccelByteWarsPlayerState*>(NewPlayer->PlayerState))
	{
		if (PlayerState->bShouldKick)
		{
			GameSession->KickPlayer(NewPlayer, FText::FromString("Max player reached"));
			GAMEMODE_LOG("Player did not registered in Teams data. Max registered players reached. Kicking this player");
		}
	}
}

int32 AAccelByteWarsGameModeBase::AddPlayerScore(APlayerState* PlayerState, const float InScore, const bool bAddKillCount)
{
	AAccelByteWarsPlayerState* AccelByteWarsPlayerState = static_cast<AAccelByteWarsPlayerState*>(PlayerState);
	if (!AccelByteWarsPlayerState)
	{
		GAMEMODE_LOG("PlayerState is not derived from AAccelByteWarsPlayerState. Operation cancelled")
		return INDEX_NONE;
	}

	FGameplayPlayerData* PlayerData =
		ByteWarsGameState->GetPlayerDataById(PlayerState->GetUniqueId(),GetControllerId(PlayerState));
	if (!PlayerData)
	{
		GAMEMODE_LOG("Player is not in Teams data. Add player to team via AddToTeam. Operation cancelled")
		return INDEX_NONE;
	}

	// set score in PlayerState and GameState
	const float FinalScore = AccelByteWarsPlayerState->GetScore() + InScore;
	AccelByteWarsPlayerState->SetScore(FinalScore);
	PlayerData->Score = FinalScore;

	// increase kill count
	if (bAddKillCount)
	{
		AccelByteWarsPlayerState->KillCount++;
		PlayerData->KillCount = AccelByteWarsPlayerState->KillCount;
	}

	return AccelByteWarsPlayerState->GetScore();
}

int32 AAccelByteWarsGameModeBase::DecreasePlayerLife(APlayerState* PlayerState, const uint8 Decrement)
{
	AAccelByteWarsPlayerState* AccelByteWarsPlayerState = static_cast<AAccelByteWarsPlayerState*>(PlayerState);
	if (!AccelByteWarsPlayerState)
	{
		GAMEMODE_LOG("PlayerState is not derived from AAccelByteWarsPlayerState. Operation cancelled")
		return INDEX_NONE;
	}

	FGameplayPlayerData* PlayerData =
		ByteWarsGameState->GetPlayerDataById(PlayerState->GetUniqueId(),GetControllerId(PlayerState));
	if (!PlayerData)
	{
		GAMEMODE_LOG("Player is not in Teams data. Add player to team via AddToTeam. Operation cancelled")
		return INDEX_NONE;
	}

	// decrease life num in PlayerState
	AccelByteWarsPlayerState->NumLivesLeft -= Decrement;

	// match life num in GameState to PlayerState
	PlayerData->NumLivesLeft = AccelByteWarsPlayerState->NumLivesLeft;

	return AccelByteWarsPlayerState->NumLivesLeft;
}

void AAccelByteWarsGameModeBase::ResetGameData()
{
	ByteWarsGameState->Teams.Empty();
}

void AAccelByteWarsGameModeBase::TriggerServerTravel(TSoftObjectPtr<UWorld> Level)
{
	const FString Url = Level.GetLongPackageName();
	GetWorld()->ServerTravel(Url);
}

void AAccelByteWarsGameModeBase::PlayerSetup(APlayerController* PlayerController) const
{
	// failsafe
	if (!PlayerController) return;

	AAccelByteWarsPlayerState* PlayerState = static_cast<AAccelByteWarsPlayerState*>(PlayerController->PlayerState);
	if (!PlayerState) return;

	int32 TeamId = INDEX_NONE;
	const FUniqueNetIdRepl PlayerUniqueId = GetPlayerUniqueNetId(PlayerController);
	const int32 ControllerId = GetControllerId(PlayerState);

	// check for a match in GameState's Teams data
	if (const FGameplayPlayerData* PlayerData =
		ByteWarsGameState->GetPlayerDataById(PlayerState->GetUniqueId(), ControllerId))
	{
		// found, restore data
		TeamId = PlayerData->TeamId;
		PlayerState->SetScore(PlayerData->Score);
		PlayerState->TeamId = TeamId;
		PlayerState->NumLivesLeft = PlayerData->NumLivesLeft;
		PlayerState->KillCount = PlayerData->KillCount;
		PlayerState->TeamColor = ByteWarsGameInstance->GetTeamColor(TeamId);

#if UE_BUILD_DEVELOPMENT
		const FString Identity = PlayerUniqueId.GetUniqueNetId().IsValid() ?
			PlayerUniqueId.GetUniqueNetId()->ToDebugString() : PlayerController->PlayerState->GetPlayerName();
		GAMEMODE_LOG("Found player's (%s) data in existing PlayerDatas. Assigning team: %d", *Identity, TeamId);
#endif
	}
	// flag to kick player if player's data was not found and max players reached (based on registered players)
	else
	{
		if (ByteWarsGameState->GetRegisteredPlayersNum() >= ByteWarsGameInstance->GameSetup.MaxPlayers)
		{
			// kick can happen as early as PostLogin
			PlayerState->bShouldKick = true;
			return;
		}
	}

	// if no match found, assign player to a new team or least populated team
	if (TeamId == INDEX_NONE)
	{
		switch (ByteWarsGameInstance->GameSetup.GameModeType)
		{
		case EGameModeType::FFA:
			// assign to a new team
			TeamId = ByteWarsGameState->Teams.Num();
			break;
		case EGameModeType::TDM:
			// check if max team reached
			if (ByteWarsGameState->Teams.Num() >= ByteWarsGameInstance->GameSetup.MaxTeamNum)
			{
				// assign to the least populated team
				uint8 CurrentTeamMemberNum = UINT8_MAX;
				TeamId = 0;
				for (const FGameplayTeamData& Team : ByteWarsGameState->Teams)
				{
					if (Team.TeamMembers.Num() < CurrentTeamMemberNum)
					{
						CurrentTeamMemberNum = Team.TeamMembers.Num();
						TeamId = Team.TeamId;
					}
				}
			}
			else
			{
				// assign to a new team
				TeamId = ByteWarsGameState->Teams.Num();
			}
			break;
		default: ;
		}

		// set player's state data
		PlayerState->TeamId = TeamId;
		PlayerState->TeamColor = ByteWarsGameInstance->GetTeamColor(TeamId);
		PlayerState->SetScore(0.0f);
        PlayerState->NumLivesLeft = INDEX_NONE;
        PlayerState->KillCount = 0;

#if UE_BUILD_DEVELOPMENT
		const FString Identity = PlayerUniqueId.GetUniqueNetId().IsValid() ?
			PlayerUniqueId.GetUniqueNetId()->ToDebugString() : PlayerController->PlayerState->GetPlayerName();
		GAMEMODE_LOG("No player's (%s) data found. Assigning team: %d", *Identity, TeamId);
#endif
	}

	// add player to team
	ByteWarsGameState->AddPlayerToTeam(
		TeamId,
		PlayerUniqueId,
		PlayerState->NumLivesLeft,
		ControllerId,
		PlayerState->GetScore(),
		PlayerState->KillCount);
}

FUniqueNetIdRepl AAccelByteWarsGameModeBase::GetPlayerUniqueNetId(const APlayerController* PlayerController)
{
	FUniqueNetIdRepl NetId;

	if (PlayerController->IsLocalController())
	{
		if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			NetId = LocalPlayer->GetPreferredUniqueNetId();
		}
	}
	else
	{
		NetId = PlayerController->PlayerState->GetUniqueId();
	}

	return NetId;
}

int32 AAccelByteWarsGameModeBase::GetControllerId(const APlayerState* PlayerState)
{
	int32 ControllerId = 0;
	if (const APlayerController* PC = PlayerState->GetPlayerController())
	{
		if (const ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			ControllerId = LP->GetLocalPlayerIndex();
		}
	}
	return ControllerId;
}
