// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Player/AccelByteWarsPlayerState.h"

#include "AccelByteWarsPlayerController.h"
#include "Core/Utilities/AccelByteWarsUtilityLog.h"
#include "Net/UnrealNetwork.h"

void AAccelByteWarsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAccelByteWarsPlayerState, AvatarURL);
	DOREPLIFETIME(AAccelByteWarsPlayerState, TeamColor);
	DOREPLIFETIME(AAccelByteWarsPlayerState, TeamId);
	DOREPLIFETIME(AAccelByteWarsPlayerState, MissilesFired);
	DOREPLIFETIME(AAccelByteWarsPlayerState, KillCount);
	DOREPLIFETIME(AAccelByteWarsPlayerState, NumLivesLeft);
	DOREPLIFETIME(AAccelByteWarsPlayerState, bPendingTeamAssignment);
	DOREPLIFETIME(AAccelByteWarsPlayerState, NumKilledAttemptInSingleLifetime);
}

void AAccelByteWarsPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (const AAccelByteWarsPlayerController* PlayerController = Cast<AAccelByteWarsPlayerController>(C))
	{
		if (HasLocalNetOwner())
		{
			PlayerController->LoadingPlayerAssignment();
		}
	}
}

void AAccelByteWarsPlayerState::RepNotify_PendingTeamAssignment()
{
	if (const AAccelByteWarsPlayerController* PlayerController = Cast<AAccelByteWarsPlayerController>(GetOwningController()))
	{
		if (HasLocalNetOwner())
		{
			PlayerController->LoadingPlayerAssignment();
		}
	}
}
