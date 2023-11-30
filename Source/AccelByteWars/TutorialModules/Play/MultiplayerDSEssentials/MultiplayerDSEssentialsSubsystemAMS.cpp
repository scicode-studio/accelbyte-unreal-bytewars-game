﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.


#include "MultiplayerDSEssentialsSubsystemAMS.h"

#include "MultiplayerDSEssentialsLog.h"
#include "OnlineSubsystemUtils.h"
#include "Core/System/AccelByteWarsGameSession.h"

void UMultiplayerDSEssentialsSubsystemAMS::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AAccelByteWarsGameSession::OnRegisterServerDelegates.AddUObject(this, &ThisClass::RegisterServer);
	AAccelByteWarsGameSession::OnUnregisterServerDelegates.AddUObject(this, &ThisClass::UnregisterServer);

	ABSessionInt = StaticCastSharedPtr<FOnlineSessionV2AccelByte>(Online::GetSessionInterface());
	ensure(ABSessionInt);
	
	ABSessionInt->OnAMSDrainReceivedDelegates.AddUObject(this, &ThisClass::OnAMSDrainReceived);
}

void UMultiplayerDSEssentialsSubsystemAMS::Deinitialize()
{
	Super::Deinitialize();

	AAccelByteWarsGameSession::OnRegisterServerDelegates.RemoveAll(this);
	AAccelByteWarsGameSession::OnUnregisterServerDelegates.RemoveAll(this);

	ABSessionInt->OnAMSDrainReceivedDelegates.RemoveAll(this);
}

void UMultiplayerDSEssentialsSubsystemAMS::RegisterServer(const FName SessionName)
{
	UE_LOG_MultiplayerDSEssentials(Verbose, TEXT("called"))

	// safety
	if (!ABSessionInt)
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Session interface null"))
		OnRegisterServerComplete(false);
		return;
	}
	if (!IsRunningDedicatedServer())
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Is not DS"));
		OnRegisterServerComplete(false);
		return;
	}

	if (bServerAlreadyRegister)
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Already registered"));
		OnRegisterServerComplete(false);
		return;
	}

	ABSessionInt->RegisterServer(SessionName, FOnRegisterServerComplete::CreateUObject(
		this, &ThisClass::OnRegisterServerComplete));
}

void UMultiplayerDSEssentialsSubsystemAMS::OnRegisterServerComplete(const bool bSucceeded)
{
	UE_LOG_MultiplayerDSEssentials(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))

	if (bSucceeded)
	{
		bServerAlreadyRegister = true;

		// Set server as ready.
		SendServerReady();
	}
}

void UMultiplayerDSEssentialsSubsystemAMS::UnregisterServer(const FName SessionName)
{
	UE_LOG_MultiplayerDSEssentials(Verbose, TEXT("called"))

	// safety
	if (!ABSessionInt)
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Session interface null"))
		OnUnregisterServerComplete(false);
		return;
	}
	if (!IsRunningDedicatedServer())
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Is not DS"));
		OnUnregisterServerComplete(false);
		return;
	}

	ABSessionInt->UnregisterServer(SessionName, FOnUnregisterServerComplete::CreateUObject(
		this, &ThisClass::OnUnregisterServerComplete));
	bUnregisterServerRunning = true;
}

void UMultiplayerDSEssentialsSubsystemAMS::OnUnregisterServerComplete(const bool bSucceeded)
{
	UE_LOG_MultiplayerDSEssentials(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))

	bUnregisterServerRunning = false;

	FPlatformMisc::RequestExit(false);
}

void UMultiplayerDSEssentialsSubsystemAMS::SendServerReady() 
{
	if (!ABSessionInt)
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Session interface null"));
		OnSendServerReadyComplete(false);
		return;
	}

	if (!IsRunningDedicatedServer())
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Is not DS"));
		OnSendServerReadyComplete(false);
		return;
	}

	ABSessionInt->SendServerReady(TEXT(""), FOnRegisterServerComplete::CreateUObject(this, &ThisClass::OnSendServerReadyComplete));
}

void UMultiplayerDSEssentialsSubsystemAMS::OnSendServerReadyComplete(const bool bSucceeded)
{
	if (bSucceeded) 
	{
		UE_LOG_MultiplayerDSEssentials(Log, TEXT("Success to set AMS server ready state."));
	}
	else 
	{
		UE_LOG_MultiplayerDSEssentials(Warning, TEXT("Failed to set AMS server ready state"));
	}
}

void UMultiplayerDSEssentialsSubsystemAMS::OnAMSDrainReceived()
{
	UE_LOG_MultiplayerDSEssentials(Log, TEXT("Received AMS drain message; Shutting down the server now!"));

	OnUnregisterServerComplete(true);
}