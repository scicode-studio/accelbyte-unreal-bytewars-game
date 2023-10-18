// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.


#include "SessionEssentialsOnlineSession.h"

#include "OnlineSubsystemAccelByteSessionSettings.h"
#include "SessionEssentialsLog.h"

void USessionEssentialsOnlineSession::RegisterOnlineDelegates()
{
	Super::RegisterOnlineDelegates();

	GetSessionInt()->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete));
	GetSessionInt()->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));
	GetABSessionInt()->AddOnSendSessionInviteCompleteDelegate_Handle(
		FOnSendSessionInviteCompleteDelegate::CreateUObject(this, &ThisClass::OnSendSessionInviteComplete));
	GetSessionInt()->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLeaveSessionComplete));

	GetABSessionInt()->AddOnV2SessionInviteReceivedDelegate_Handle(
		FOnV2SessionInviteReceivedDelegate::CreateUObject(this, &ThisClass::OnSessionInviteReceived));
	GetABSessionInt()->AddOnSessionParticipantsChangeDelegate_Handle(
		FOnSessionParticipantsChangeDelegate::CreateUObject(this, &ThisClass::OnSessionParticipantsChange));
}

void USessionEssentialsOnlineSession::ClearOnlineDelegates()
{
	Super::ClearOnlineDelegates();

	GetSessionInt()->ClearOnCreateSessionCompleteDelegates(this);
	GetSessionInt()->ClearOnJoinSessionCompleteDelegates(this);
	GetABSessionInt()->ClearOnSendSessionInviteCompleteDelegates(this);
	GetSessionInt()->ClearOnDestroySessionCompleteDelegates(this);

	GetABSessionInt()->ClearOnV2SessionInviteReceivedDelegates(this);
	GetABSessionInt()->ClearOnSessionParticipantsChangeDelegates(this);
}

FNamedOnlineSession* USessionEssentialsOnlineSession::GetSession(const FName SessionName)
{
	return GetSessionInt()->GetNamedSession(SessionName);
}

EAccelByteV2SessionType USessionEssentialsOnlineSession::GetSessionType(const FName SessionName)
{
	const FNamedOnlineSession* OnlineSession = GetSession(SessionName);
	if (!OnlineSession)
	{
		return EAccelByteV2SessionType::Unknown;
	}

	const FOnlineSessionSettings& OnlineSessionSettings = OnlineSession->SessionSettings;

	return GetABSessionInt()->GetSessionTypeFromSettings(OnlineSessionSettings);
}

FName USessionEssentialsOnlineSession::GetPredefinedSessionNameFromType(const EAccelByteV2SessionType SessionType)
{
	FName SessionName = FName();

	switch (SessionType)
	{
	case EAccelByteV2SessionType::GameSession:
		SessionName = GameSessionName;
		break;
	case EAccelByteV2SessionType::PartySession:
		SessionName = PartySessionName;
		break;
	default: ;
	}

	return SessionName;
}

void USessionEssentialsOnlineSession::CreateSession(
	const int32 LocalUserNum,
	FName SessionName,
	FOnlineSessionSettings SessionSettings,
	const EAccelByteV2SessionType SessionType,
	const FString& SessionTemplateName)
{
	UE_LOG_SESSIONESSENTIALS(Verbose, TEXT("called"))

	// safety
	if (!GetSessionInt())
	{
		UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Session interface is null"))
		ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
		{
			OnCreateSessionComplete(SessionName, false);
		}));
		return;
	}
	if (SessionTemplateName.IsEmpty())
	{
		UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Session Template Name can't be empty"))
		ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
		{
			OnCreateSessionComplete(SessionName, false);
		}));
		return;
	}

#pragma region "Setup Session Settings"
	// Session Template Name
	SessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, SessionTemplateName);

	// Session type
	if (SessionType != EAccelByteV2SessionType::Unknown)
	{
		SessionSettings.Set(SETTING_SESSION_TYPE, GetPredefinedSessionNameFromType(SessionType).ToString());
	}

	if (SessionType == EAccelByteV2SessionType::GameSession)
	{
		// Check for DS version override.
		const FString OverriddenDSVersion = UTutorialModuleOnlineUtility::GetDedicatedServerVersionOverride();
		if (!OverriddenDSVersion.IsEmpty())
		{
			SessionSettings.Set(SETTING_GAMESESSION_CLIENTVERSION, OverriddenDSVersion);
		}

		// Set local server name for matchmaking request if any.
		// This is useful if you want to try matchmaking using local dedicated server.
		FString ServerName;
		FParse::Value(FCommandLine::Get(), TEXT("-ServerName="), ServerName);
		if (!ServerName.IsEmpty())
		{
			UE_LOG_SESSIONESSENTIALS(Log, TEXT("Requesting to use server with name: %s"), *ServerName)
			SessionSettings.Set(SETTING_GAMESESSION_SERVERNAME, ServerName);
		}
	}
#pragma endregion

	// if session exist locally -> destroy session first
	if (GetSession(SessionName))
	{
		UE_LOG_SESSIONESSENTIALS(Log, TEXT("Session exist locally, leaving session first"))

		// remove from delegate if exist
		if (OnLeaveSessionForCreateSessionCompleteDelegateHandle.IsValid())
		{
			OnLeaveSessionCompleteDelegates.Remove(OnLeaveSessionForCreateSessionCompleteDelegateHandle);
			OnLeaveSessionForCreateSessionCompleteDelegateHandle.Reset();
		}

		OnLeaveSessionForCreateSessionCompleteDelegateHandle = OnLeaveSessionCompleteDelegates.AddUObject(this, &ThisClass::OnLeaveSessionForCreateSessionComplete, LocalUserNum, SessionSettings);
		LeaveSession(SessionName);
	}
	else
	{
		if (!GetSessionInt()->CreateSession(LocalUserNum, SessionName, SessionSettings))
		{
			UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Failed to execute"))
			ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
			{
				OnCreateSessionComplete(SessionName, false);
			}));
		}
	}
}

void USessionEssentialsOnlineSession::JoinSession(
	const int32 LocalUserNum,
	FName SessionName,
	const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG_SESSIONESSENTIALS(Verbose, TEXT("called"))

	// safety
	if (!GetSessionInt())
	{
		UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Session interface null"))
		ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
		{
			OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		}));
		return;
	}

	// If session exist -> destroy first then join
	if (GetSession(SessionName))
	{
		// remove from delegate if exist
		if (OnLeaveSessionForJoinSessionCompleteDelegateHandle.IsValid())
		{
			OnLeaveSessionCompleteDelegates.Remove(OnLeaveSessionForJoinSessionCompleteDelegateHandle);
			OnLeaveSessionForJoinSessionCompleteDelegateHandle.Reset();
		}

		OnLeaveSessionForJoinSessionCompleteDelegateHandle = OnLeaveSessionCompleteDelegates.AddUObject(this, &ThisClass::OnLeaveSessionForJoinSessionComplete, LocalUserNum, SearchResult);
		LeaveSession(SessionName);
	}
	else
	{
		if (!GetSessionInt()->JoinSession(LocalUserNum, SessionName, SearchResult))
		{
			UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Failed to execute"))
			ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
			{
				OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
			}));
		}
	}
}

void USessionEssentialsOnlineSession::SendSessionInvite(
	const int32 LocalUserNum,
	FName SessionName,
	const FUniqueNetIdPtr Invitee)
{
	UE_LOG_SESSIONESSENTIALS(Verbose, TEXT("Called"));

	if (!Invitee.IsValid())
	{
		UE_LOG_SESSIONESSENTIALS(Log, TEXT("Invitee net id is invalid. Cancelling operation"));
		return;
	}

	GetABSessionInt()->SendSessionInviteToFriend(LocalUserNum, SessionName, *Invitee.Get());
}

void USessionEssentialsOnlineSession::RejectSessionInvite(
	const int32 LocalUserNum,
	const FOnlineSessionInviteAccelByte& Invite)
{
	UE_LOG_SESSIONESSENTIALS(Verbose, TEXT("Called"));

	const FUniqueNetIdPtr LocalUserNetId = GetLocalPlayerUniqueNetId(GetPlayerControllerByLocalUserNum(LocalUserNum));
	if (!LocalUserNetId.IsValid())
	{
		UE_LOG_SESSIONESSENTIALS(Log, TEXT("Local User net id is invalid. Cancelling operation"));
		return;
	}

	GetABSessionInt()->RejectInvite(
		*LocalUserNetId.Get(),
		Invite,
		FOnRejectSessionInviteComplete::CreateUObject(this, &ThisClass::OnRejectSessionInviteComplete));
}

void USessionEssentialsOnlineSession::LeaveSession(FName SessionName)
{
	UE_LOG_SESSIONESSENTIALS(Verbose, TEXT("called"))

	// safety
	if (!GetSessionInt())
	{
		UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Session interface null"))
		ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
		{
			OnLeaveSessionComplete(SessionName, false);
		}));
		return;
	}

	if (GetSession(SessionName))
	{
		if (!GetABSessionInt()->DestroySession(SessionName))
		{
			UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Failed to execute"))
			ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
			{
				OnLeaveSessionComplete(SessionName, false);
			}));
		}
		else
		{
			bLeaveSessionRunning = true;
		}
	}
	else
	{
		UE_LOG_SESSIONESSENTIALS(Log, TEXT("Not in session"))
		ExecuteNextTick(FSimpleDelegate::CreateWeakLambda(this, [this, SessionName]()
		{
			OnLeaveSessionComplete(SessionName, true);
		}));
	}
}

void USessionEssentialsOnlineSession::OnCreateSessionComplete(FName SessionName, bool bSucceeded)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))

	OnCreateSessionCompleteDelegates.Broadcast(SessionName, bSucceeded);
}

void USessionEssentialsOnlineSession::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(Result == EOnJoinSessionCompleteResult::Success ? "TRUE" : "FALSE"))

	OnJoinSessionCompleteDelegates.Broadcast(SessionName, Result);
}

void USessionEssentialsOnlineSession::OnSendSessionInviteComplete(
	const FUniqueNetId& LocalSenderId,
	FName SessionName,
	bool bSucceeded,
	const FUniqueNetId& InviteeId)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE" : "FALSE"))

	OnSendSessionInviteCompleteDelegates.Broadcast(LocalSenderId, SessionName, bSucceeded, InviteeId);
}

void USessionEssentialsOnlineSession::OnRejectSessionInviteComplete(bool bSucceeded)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE" : "FALSE"))

	OnRejectSessionInviteCompleteDelegates.Broadcast(bSucceeded);
}

void USessionEssentialsOnlineSession::OnLeaveSessionComplete(FName SessionName, bool bSucceeded)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))

	bLeaveSessionRunning = false;
	OnLeaveSessionCompleteDelegates.Broadcast(SessionName, bSucceeded);
}

void USessionEssentialsOnlineSession::OnSessionInviteReceived(
	const FUniqueNetId& UserId,
	const FUniqueNetId& FromId,
	const FOnlineSessionInviteAccelByte& Invite)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("from: %s"), *FromId.ToDebugString())

	OnSessionInviteReceivedDelegates.Broadcast(UserId, FromId, Invite);
}

void USessionEssentialsOnlineSession::OnSessionParticipantsChange(
	FName SessionName,
	const FUniqueNetId& Member,
	bool bJoined)
{
	UE_LOG_SESSIONESSENTIALS(
		Log,
		TEXT("session name: %s | Member: %s [%s]"),
		*SessionName.ToString(),
		*Member.ToDebugString(),
		*FString(bJoined ? "Joined" : "Left"))

	OnSessionParticipantsChangeDelegates.Broadcast(SessionName, Member, bJoined);
}

void USessionEssentialsOnlineSession::OnLeaveSessionForCreateSessionComplete(
	FName SessionName,
	bool bSucceeded,
	const int32 LocalUserNum,
	const FOnlineSessionSettings SessionSettings)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))
	OnLeaveSessionCompleteDelegates.Remove(OnLeaveSessionForCreateSessionCompleteDelegateHandle);

	if (bSucceeded)
	{
		if (!GetSessionInt()->CreateSession(LocalUserNum, SessionName, SessionSettings))
		{
			UE_LOG_SESSIONESSENTIALS(Warning, TEXT("Failed to execute"))
			OnCreateSessionComplete(SessionName, false);
		}
	}
	else
	{
		// Leave Session failed, execute complete delegate as failed
		OnCreateSessionComplete(SessionName, false);
	}
}

void USessionEssentialsOnlineSession::OnLeaveSessionForJoinSessionComplete(
	FName SessionName,
	bool bSucceeded,
	const int32 LocalUserNum,
	const FOnlineSessionSearchResult SearchResult)
{
	UE_LOG_SESSIONESSENTIALS(Log, TEXT("succeeded: %s"), *FString(bSucceeded ? "TRUE": "FALSE"))
	OnLeaveSessionCompleteDelegates.Remove(OnLeaveSessionForJoinSessionCompleteDelegateHandle);

	if (bSucceeded)
	{
		if (!GetSessionInt()->JoinSession(LocalUserNum, SessionName, SearchResult))
		{
			UE_LOG_SESSIONESSENTIALS(Warning, TEXT("failed to execute"))
			OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		}
	}
	else
	{
		// Leave Session failed, execute complete delegate as failed
		OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	}
}
