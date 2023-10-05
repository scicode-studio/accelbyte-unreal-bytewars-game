// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "TutorialModules/PlayingWithParty/PlayWithPartySubsystem.h"

#include "TutorialModules/OnlineSessionUtils/AccelByteWarsOnlineSessionBase.h"

#include "OnlineSubsystemAccelByteSessionSettings.h"
#include "OnlineSubsystemUtils.h"

#include "Core/System/AccelByteWarsGameInstance.h"
#include "Core/UI/AccelByteWarsBaseUI.h"
#include "Core/UI/Components/Prompt/PromptSubsystem.h"

#include "JsonObjectConverter.h"

void UPlayWithPartySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (GetSessionInterface())
    {
        // Bind party matchmaking events.
        GetSessionInterface()->OnMatchmakingStartedDelegates.AddUObject(this, &ThisClass::OnStartPartyMatchmakingComplete);
        GetSessionInterface()->OnMatchmakingCompleteDelegates.AddUObject(this, &ThisClass::OnPartyMatchmakingComplete);
        GetSessionInterface()->OnMatchmakingCanceledDelegates.AddUObject(this, &ThisClass::OnPartyMatchmakingCanceled);
        GetSessionInterface()->OnMatchmakingExpiredDelegates.AddUObject(this, &ThisClass::OnPartyMatchmakingExpired);

        // Bind party game session events.
        GetSessionInterface()->OnCreateSessionCompleteDelegates.AddUObject(this, &ThisClass::OnCreatePartyMatchComplete);
        GetSessionInterface()->OnJoinSessionCompleteDelegates.AddUObject(this, &ThisClass::OnJoinPartyMatchComplete);
        GetSessionInterface()->OnV2SessionInviteReceivedDelegates.AddUObject(this, &ThisClass::OnPartyMatchInviteReceived);
        GetSessionInterface()->OnDestroySessionCompleteDelegates.AddUObject(this, &ThisClass::OnLeavePartyMatchComplete);
    }

    // Add party validation to online session related UIs.
    if (GetOnlineSession()) 
    {
        GetOnlineSession()->ValidateToStartSession.Unbind();
        GetOnlineSession()->ValidateToStartSession.BindUObject(this, &ThisClass::ValidateToStartSession);

        GetOnlineSession()->ValidateToStartMatchmaking.Unbind();
        GetOnlineSession()->ValidateToStartMatchmaking.BindUObject(this, &ThisClass::ValidateToStartMatchmaking);

        GetOnlineSession()->ValidateToJoinSession.Unbind();
        GetOnlineSession()->ValidateToJoinSession.BindUObject(this, &ThisClass::ValidateToJoinSession);
    }
}

void UPlayWithPartySubsystem::Deinitialize()
{
    Super::Deinitialize();

    if (GetSessionInterface())
    {
        // Unbind party matchmaking events.
        GetSessionInterface()->OnMatchmakingStartedDelegates.RemoveAll(this);
        GetSessionInterface()->OnMatchmakingCompleteDelegates.RemoveAll(this);
        GetSessionInterface()->OnMatchmakingCanceledDelegates.RemoveAll(this);
        GetSessionInterface()->OnMatchmakingExpiredDelegates.RemoveAll(this);

        // Unbind party game session events.
        GetSessionInterface()->OnCreateSessionCompleteDelegates.RemoveAll(this);
        GetSessionInterface()->OnJoinSessionCompleteDelegates.RemoveAll(this);
        GetSessionInterface()->OnV2SessionInviteReceivedDelegates.RemoveAll(this);
        GetSessionInterface()->OnDestroySessionCompleteDelegates.RemoveAll(this);
    }

    // Remove party validation to online session related UIs.
    if (GetOnlineSession())
    {
        if (GetOnlineSession()->ValidateToStartSession.GetUObject() == this) 
        {
            GetOnlineSession()->ValidateToStartSession.Unbind();
        }

        if (GetOnlineSession()->ValidateToStartMatchmaking.GetUObject() == this)
        {
            GetOnlineSession()->ValidateToStartMatchmaking.Unbind();
        }

        if (GetOnlineSession()->ValidateToJoinSession.GetUObject() == this)
        {
            GetOnlineSession()->ValidateToJoinSession.Unbind();
        }
    }
}

UAccelByteWarsOnlineSessionBase* UPlayWithPartySubsystem::GetOnlineSession() const
{
    if (!GetGameInstance())
    {
        return nullptr;
    }

    return Cast<UAccelByteWarsOnlineSessionBase>(GetGameInstance()->GetOnlineSession());
}

FOnlineSessionV2AccelBytePtr UPlayWithPartySubsystem::GetSessionInterface() const
{
    const UWorld* World = GetWorld();
    if (!ensure(World))
    {
        return nullptr;
    }

    return StaticCastSharedPtr<FOnlineSessionV2AccelByte>(Online::GetSessionInterface(World));
}

FOnlineIdentityAccelBytePtr UPlayWithPartySubsystem::GetIdentityInterface() const
{
    const UWorld* World = GetWorld();
    if (!ensure(World))
    {
        return nullptr;
    }

    return StaticCastSharedPtr<FOnlineIdentityAccelByte>(Online::GetIdentityInterface(World));
}

UPromptSubsystem* UPlayWithPartySubsystem::GetPromptSubystem()
{
    if (UAccelByteWarsGameInstance* GameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance()))
    {
        return GameInstance->GetSubsystem<UPromptSubsystem>();
    }

    return nullptr;
}

void UPlayWithPartySubsystem::OnStartPartyMatchmakingComplete()
{
    if (!GetSessionInterface() || !GetOnlineSession()) 
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on start party matchmaking completed. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetSessionInterface()->IsInPartySession() ||
        GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Party matchmaking started."));

    /* Show notification that the party matchmaking is started.
     * Only show the notification if a party member.*/
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }
    if (GetOnlineSession()->IsPartyLeader(UserId))
    {
        return;
    }

    if (GetPromptSubystem())
    {
        GetPromptSubystem()->ShowLoading(PARTY_MATCHMAKING_STARTED_MESSAGE);
    }
}

void UPlayWithPartySubsystem::OnPartyMatchmakingComplete(FName SessionName, bool bSucceeded)
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on party matchmaking completed. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetSessionInterface()->IsInPartySession() ||
        GetOnlineSession()->GetPartyMembers().Num() <= 1 ||
        !GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession).IsEqual(SessionName))
    {
        return;
    }

    if (bSucceeded)
    {
        UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Party matchmaking found. Currently joining the match."));
    }
    else 
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Party matchmaking failed."));
    }

    /* Show notification that the party matchmaking is completed.
     * Only show the notification if a party member.*/
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }
    if (GetOnlineSession()->IsPartyLeader(UserId))
    {
        return;
    }

    if (GetPromptSubystem())
    {
        if (bSucceeded)
        {
            GetPromptSubystem()->ShowLoading(PARTY_MATCHMAKING_SUCCESS_MESSAGE);
        }
        else 
        {
            GetPromptSubystem()->HideLoading();
            GetPromptSubystem()->PushNotification(PARTY_MATCHMAKING_FAILED_MESSAGE, FString(""));
        }
    }
}

void UPlayWithPartySubsystem::OnPartyMatchmakingCanceled()
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on party matchmaking canceled. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetSessionInterface()->IsInPartySession() ||
        GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Party Matchmaking is canceled."));

    /* Show notification that the party matchmaking is canceled.
     * Only show the notification if a party member.*/
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }
    if (GetOnlineSession()->IsPartyLeader(UserId))
    {
        return;
    }

    if (GetPromptSubystem())
    {
        GetPromptSubystem()->HideLoading();
        GetPromptSubystem()->PushNotification(PARTY_MATCHMAKING_CANCELED_MESSAGE, FString(""));
    }
}

void UPlayWithPartySubsystem::OnPartyMatchmakingExpired()
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on party matchmaking expired. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetSessionInterface()->IsInPartySession() ||
        GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Party matchmaking expired."));

    /* Show notification that the party matchmaking is expired.
     * Only show the notification if a party member.*/
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }
    if (GetOnlineSession()->IsPartyLeader(UserId))
    {
        return;
    }

    if (GetPromptSubystem())
    {
        GetPromptSubystem()->HideLoading();
        GetPromptSubystem()->PushNotification(PARTY_MATCHMAKING_EXPIRED_MESSAGE, FString(""));
    }
}

void UPlayWithPartySubsystem::InvitePartyMembersToJoinPartyMatch(const FUniqueNetIdPtr LeaderUserId)
{
    if (!LeaderUserId)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot invite party members to join party match. Party leader is not valid."));
        return;
    }

    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on invite party members to join party match. Interfaces or online session are not valid."));
        return;
    }

    if (!GetOnlineSession()->IsPartyLeader(LeaderUserId))
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot invite party members to join party match. Inviter is not the party leader."));
        return;
    }

    // Not necessary to send party match invitation if there is only one member.
    if (GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    // Send party match invitation to each party members.
    for (auto& Member : GetOnlineSession()->GetPartyMembers())
    {
        if (GetOnlineSession()->IsPartyLeader(Member))
        {
            continue;
        }

        if (FUniqueNetIdAccelByteUserPtr MemberABId = StaticCastSharedRef<const FUniqueNetIdAccelByteUser>(Member))
        {
            UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Send party match invitation to: %s."), *MemberABId->GetAccelByteId());
            GetSessionInterface()->SendSessionInviteToFriend(
                LeaderUserId.ToSharedRef().Get(),
                GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession),
                Member.Get());
        }
    }
}

void UPlayWithPartySubsystem::OnPartyMatchInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FOnlineSessionInviteAccelByte& Invite)
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on party match invite received. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match and if the invitation is not from the party leader.
    if (Invite.SessionType != EAccelByteV2SessionType::GameSession ||
        !GetOnlineSession()->IsPartyLeader(FromId.AsShared()) ||
        GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    // Abort if the receiver is the party leader.
    if (GetOnlineSession()->IsPartyLeader(UserId.AsShared()))
    {
        return;
    }

    // Join party match.
    const APlayerController* PC = GetOnlineSession()->GetPlayerControllerByUniqueNetId(UserId.AsShared());
    if (!PC)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot join a party match invitation from party leader. PlayerController is not valid."));
        return;
    }

    const int32 LocalUserNum = GetOnlineSession()->GetLocalUserNumFromPlayerController(PC);

    UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Received a party match invitation from party leader. Joining the party match."));

    if (GetPromptSubystem())
    {
        GetPromptSubystem()->ShowLoading(JOIN_PARTY_MATCH_MESSAGE);
    }

    GetOnlineSession()->JoinSession(LocalUserNum,
        GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession),
        Invite.Session);
}

void UPlayWithPartySubsystem::UpdatePartyMemberGameSession(const FUniqueNetIdPtr MemberUserId)
{
    if (!MemberUserId)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Party member is not valid."));
        return;
    }

    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Interfaces or online session are not valid."));
        return;
    }

    FString GameSessionId;
    FNamedOnlineSession* GameSession = GetSessionInterface()->GetNamedSession(
        GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession));
    if (GameSession)
    {
        GameSessionId = GameSession->GetSessionIdStr();
    }

    FNamedOnlineSession* PartySession = GetSessionInterface()->GetPartySession();
    if (!PartySession)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Party session is not valid."));
        return;
    }

    // Construct party game session data.
    const FUniqueNetIdAccelByteUserRef MemberUserABId = StaticCastSharedRef<const FUniqueNetIdAccelByteUser>(MemberUserId.ToSharedRef());
    TSharedPtr<FJsonObject> MembersGameSessionId = MakeShareable(new FJsonObject);
    FOnlineSessionSetting PartyGameSessionSetting;
    if (PartySession->SessionSettings.Settings.Contains(PARTY_MEMBERS_GAME_SESSION_ID))
    {
        PartyGameSessionSetting = PartySession->SessionSettings.Settings[PARTY_MEMBERS_GAME_SESSION_ID];
        TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(PartyGameSessionSetting.Data.ToString());
        if (!FJsonSerializer::Deserialize(JsonReader, MembersGameSessionId))
        {
            UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Failed to parse party members game session."));
            return;
        }
    }

    // Update party member game session id.
    if (!MembersGameSessionId)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Failed to parse party members game session."));
        return;
    }
    MembersGameSessionId->RemoveField(MemberUserABId->GetAccelByteId());
    if (!GameSessionId.IsEmpty())
    {
        MembersGameSessionId->SetStringField(MemberUserABId->GetAccelByteId(), GameSessionId);
    }

    // Remove invalid party member data.
    for (auto Pair : MembersGameSessionId->Values)
    {
        bool bIsValidMember = false;

        for (auto& ValidMember : PartySession->RegisteredPlayers)
        {
            const FUniqueNetIdAccelByteUserRef ValidMemberUserABId = StaticCastSharedRef<const FUniqueNetIdAccelByteUser>(ValidMember);
            if (Pair.Key.Equals(ValidMemberUserABId->GetAccelByteId()))
            {
                bIsValidMember = true;
                break;
            }
        }

        if (!bIsValidMember)
        {
            MembersGameSessionId->RemoveField(Pair.Key);
        }
    }

    // Update party game session data to the party session settings.
    FString MembersGameSessionIdStr;
    TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<TCHAR>::Create(&MembersGameSessionIdStr);
    if (!FJsonSerializer::Serialize(MembersGameSessionId.ToSharedRef(), JsonWriter))
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot update party member game session. Failed to parse party members game session."));
        return;
    }
    PartyGameSessionSetting.Data = MembersGameSessionIdStr;

    // Update party game session to store party game session data.
    PartySession->SessionSettings.Settings.Remove(PARTY_MEMBERS_GAME_SESSION_ID);
    PartySession->SessionSettings.Settings.Add(PARTY_MEMBERS_GAME_SESSION_ID, PartyGameSessionSetting);
    GetSessionInterface()->UpdateSession(
        GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::PartySession),
        PartySession->SessionSettings);
}

bool UPlayWithPartySubsystem::IsGameSessionDifferFromParty(const FUniqueNetIdPtr MemberUserId)
{
    if (!MemberUserId)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot check whether the game session is differ from party. Party member is not valid."));
        return false;
    }

    bool bResult = false;

    // Abort if interfaces and data is not valid.
    if (!GetSessionInterface() || !GetOnlineSession() || !MemberUserId)
    {
        return bResult;
    }

    // Abort if not in a party session.
    FNamedOnlineSession* PartySession = GetSessionInterface()->GetPartySession();
    if (!PartySession)
    {
        return bResult;
    }

    // Get current game session id.
    FString GameSessionId;
    FNamedOnlineSession* GameSession = GetSessionInterface()->GetNamedSession(
        GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession));
    if (GameSession)
    {
        GameSessionId = GameSession->GetSessionIdStr();
    }

    // Get party game session data.
    FOnlineSessionSetting* PartyGameSessionSetting = PartySession->SessionSettings.Settings.Find(PARTY_MEMBERS_GAME_SESSION_ID);
    if (!PartyGameSessionSetting)
    {
        return bResult;
    }

    TSharedPtr<FJsonObject> MembersGameSessionId = MakeShareable(new FJsonObject);
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(PartyGameSessionSetting->Data.ToString());
    if (!FJsonSerializer::Deserialize(JsonReader, MembersGameSessionId))
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot check whether the game session is differ from party. Failed to parse party members game session."));
        return bResult;
    }
    if (!MembersGameSessionId)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot check whether the game session is differ from party. Failed to parse party members game session."));
        return bResult;
    }

    FString MemberGameSessionIdStr;
    for (auto& Member : GetOnlineSession()->GetPartyMembers())
    {
        // Not necessary to check the player itself.
        if (Member.Get() == MemberUserId.ToSharedRef().Get())
        {
            continue;
        }

        // Check if the current game session is the same as the party.
        const FUniqueNetIdAccelByteUserRef MemberABId = StaticCastSharedRef<const FUniqueNetIdAccelByteUser>(Member);
        if (!MembersGameSessionId->TryGetStringField(MemberABId->GetAccelByteId(), MemberGameSessionIdStr))
        {
            continue;
        }
        if (!GameSessionId.Equals(MemberGameSessionIdStr))
        {
            bResult = true;
            break;
        }
    }

    return bResult;
}

void UPlayWithPartySubsystem::OnCreatePartyMatchComplete(FName SessionName, bool bSucceeded)
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on create party match completed. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession).IsEqual(SessionName) ||
        !GetSessionInterface()->IsInPartySession())
    {
        return;
    }

    if (bSucceeded) 
    {
        UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Success to create party match."));
    }
    else 
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Failed to create party match."));
    }

    // Update party member game session id.
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);

        UpdatePartyMemberGameSession(UserId);
    }

    // Invite party members to join the party match.
    if (GetOnlineSession()->IsPartyLeader(UserId)) 
    {
        InvitePartyMembersToJoinPartyMatch(UserId);
    }
}

void UPlayWithPartySubsystem::OnJoinPartyMatchComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on join party match completed. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession).IsEqual(SessionName) ||
        !GetSessionInterface()->IsInPartySession())
    {
        return;
    }

    if (Result == EOnJoinSessionCompleteResult::Type::Success)
    {
        UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Success to join party match."));
    }
    else
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Failed to join party match."));
    }

    // Update party member game session id.
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);

        UpdatePartyMemberGameSession(UserId);
    }

    // Not necessary to send invitation or show notification if there is only one party member.
    if (GetOnlineSession()->GetPartyMembers().Num() <= 1)
    {
        return;
    }

    // Send invitation to other party members if the one who joined the session is party leader.
    if (GetOnlineSession()->IsPartyLeader(UserId))
    {
        if (Result == EOnJoinSessionCompleteResult::Type::Success) 
        {
            InvitePartyMembersToJoinPartyMatch(UserId);
        }
    }
    // Show relevant notification if the one who joined the session is party member.
    else 
    {
        if (Result == EOnJoinSessionCompleteResult::Type::Success)
        {
            if (GetPromptSubystem())
            {
                GetPromptSubystem()->HideLoading();
                GetPromptSubystem()->PushNotification(JOIN_PARTY_MATCH_SUCCESS_MESSAGE, FString(""));
            }
        }
        else
        {
            if (GetPromptSubystem())
            {
                GetPromptSubystem()->HideLoading();
                GetPromptSubystem()->PushNotification(JOIN_PARTY_MATCH_FAILED_MESSAGE, FString(""));
            }
        }
    }
}

void UPlayWithPartySubsystem::OnLeavePartyMatchComplete(FName SessionName, bool bSucceeded)
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot handle on leave party match completed. Interfaces or online session are not valid."));
        return;
    }

    // Abort if not a party match.
    if (!GetOnlineSession()->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession).IsEqual(SessionName) ||
        !GetSessionInterface()->IsInPartySession())
    {
        return;
    }

    FNamedOnlineSession* PartySession = GetSessionInterface()->GetPartySession();
    if (!PartySession)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot clear party member game session upon leaving the party match. Party session is not valid."));
        return;
    }

    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }
    if (!UserId)
    {
        return;
    }

    // Update party session to clear game session data.
    UpdatePartyMemberGameSession(UserId);

    UE_LOG_PLAYINGWITHPARTY(Log, TEXT("Success to leave party match."));
}

bool UPlayWithPartySubsystem::ValidateToStartSession()
{
    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot validate to start party match. Interfaces or online session are not valid."));
        return false;
    }

    // Get current player.
    FUniqueNetIdPtr UserId = nullptr;
    if (GetIdentityInterface())
    {
        UserId = GetIdentityInterface()->GetUniquePlayerId(0);
    }

    // Only party leader is able to start party match.
    if (GetOnlineSession() && !GetOnlineSession()->IsPartyLeader(UserId))
    {
        if (GetPromptSubystem())
        {
            GetPromptSubystem()->PushNotification(PARTY_MATCH_MEMBER_SAFEGUARD_MESSAGE, FString(""));
        }
        return false;
    }

    // Only able to start party match if other party members are not in other game session.
    bool bResult = !IsGameSessionDifferFromParty(UserId);

    /* Show notification that unable to start any game session
     * if other party members are in other game session.*/
    if (!bResult && GetPromptSubystem())
    {
        GetPromptSubystem()->PushNotification(PARTY_MATCH_LEADER_SAFEGUARD_MESSAGE, FString(""));
    }

    return bResult;
}

bool UPlayWithPartySubsystem::ValidateToJoinSession(const FOnlineSessionSearchResult& SessionSearchResult)
{
    if (!ValidateToStartSession())
    {
        return false;
    }

    if (!GetSessionInterface() || !GetOnlineSession())
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot validate to join session. Interfaces or online session are not valid."));
        return false;
    }

    TSharedPtr<FOnlineSessionInfoAccelByteV2> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoAccelByteV2>(SessionSearchResult.Session.SessionInfo);
    if (!SessionInfo)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot validate to join session. Session is not valid."));
        return false;
    }

    TSharedPtr<FAccelByteModelsV2BaseSession> SessionData = SessionInfo->GetBackendSessionData();
    if (!SessionData)
    {
        UE_LOG_PLAYINGWITHPARTY(Warning, TEXT("Cannot validate to join session. Session data is not valid."));
        return false;
    }

    // Check if session slots is sufficient to join with party
    int32 ActiveMemberCount = SessionData->Members.FilterByPredicate([](FAccelByteModelsV2SessionUser Temp)
    {
        return Temp.StatusV2 == EAccelByteV2SessionMemberStatus::JOINED;
    }).Num();

    bool bResult =
        (SessionSearchResult.Session.SessionSettings.NumPublicConnections - ActiveMemberCount) >=
        GetOnlineSession()->GetPartyMembers().Num();

    // Notify that no more slots to join the session.
    if (!bResult && GetPromptSubystem())
    {
        GetPromptSubystem()->PushNotification(JOIN_PARTY_MATCH_SAFEGUARD_MESSAGE, FString(""));
    }

    return bResult;
}

bool UPlayWithPartySubsystem::ValidateToStartMatchmaking(const EGameModeType GameModeType)
{
    if (!ValidateToStartSession())
    {
        return false;
    }

    bool bResult = GameModeType == EGameModeType::FFA;

    // Notify cannot matchmaking using the specified game mode.
    if (!bResult && GetPromptSubystem())
    {
        GetPromptSubystem()->PushNotification(PARTY_MATCHMAKING_SAFEGUARD_MESSAGE, FString(""));
    }

    return bResult;
}