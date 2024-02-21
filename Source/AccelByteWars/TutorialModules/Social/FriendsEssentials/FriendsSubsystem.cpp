// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "FriendsSubsystem.h"
#include "Core/System/AccelByteWarsGameInstance.h"
#include "Core/UI/Components/Prompt/PromptSubsystem.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "AccelByteWars"

void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    // Get Online Subsystem and make sure it's valid.
    const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (!ensure(Subsystem))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("The online subsystem is invalid. Please make sure OnlineSubsystemAccelByte is enabled and DefaultPlatformService under [OnlineSubsystem] in the Engine.ini set to AccelByte."));
        return;
    }

    // Grab the reference of AccelByte User Interface and make sure it's valid.
    UserInterface = StaticCastSharedPtr<FOnlineUserAccelByte>(Subsystem->GetUserInterface());
    if (!ensure(UserInterface.IsValid()))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("User Interface is not valid."));
        return;
    }

    // Grab the reference of AccelByte Friends Interface and make sure it's valid.
    FriendsInterface = StaticCastSharedPtr<FOnlineFriendsAccelByte>(Subsystem->GetFriendsInterface());
    if (!ensure(FriendsInterface.IsValid()))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Friends Interface is not valid."));
        return;
    }

    // Grab prompt subsystem.
    UAccelByteWarsGameInstance* GameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance());
    ensure(GameInstance);

    PromptSubsystem = GameInstance->GetSubsystem<UPromptSubsystem>();
    ensure(PromptSubsystem);
}

void UFriendsSubsystem::Deinitialize()
{
    Super::Deinitialize();

    // Clear on friends changed delegate.
    for (auto& DelegateHandle : OnFriendsChangeDelegateHandles)
    {
        FriendsInterface->ClearOnFriendsChangeDelegate_Handle(DelegateHandle.Key, DelegateHandle.Value);
    }
}

FUniqueNetIdPtr UFriendsSubsystem::GetUniqueNetIdFromPlayerController(const APlayerController* PC) const
{
    if (!PC)
    {
        return nullptr;
    }

    ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    if (!LocalPlayer)
    {
        return nullptr;
    }

    return LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
}

int32 UFriendsSubsystem::GetLocalUserNumFromPlayerController(const APlayerController* PC) const
{
    int32 LocalUserNum = 0;

    if (!PC)
    {
        return LocalUserNum;
    }

    const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    if (LocalPlayer)
    {
        LocalUserNum = LocalPlayer->GetControllerId();
    }

    return LocalUserNum;
}


#pragma region Module.8a Function Definitions

void UFriendsSubsystem::GetCacheFriendList(const int32 LocalUserNum, const FOnGetCacheFriendListComplete& OnComplete)
{
    if (!ensure(FriendsInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot cache friend list. Friends Interface is not valid."));
        return;
    }
    
    // Try to get cached friend list first.
    TArray<TSharedRef<FOnlineFriend>> CachedFriendList;
    if (FriendsInterface->GetFriendsList(LocalUserNum, TEXT(""), CachedFriendList))
    {
        // Then, update the cached friends' information by querying their user information.
        TPartyMemberArray FriendIds;
        for (const TSharedRef<FOnlineFriend>& CachedFriend : CachedFriendList)
        {
            FriendIds.Add(CachedFriend.Get().GetUserId());
        }

        // Create callback to handle queried friends' user information.
        OnQueryUserInfoCompleteDelegateHandle = UserInterface->AddOnQueryUserInfoCompleteDelegate_Handle(
            LocalUserNum,
            FOnQueryUserInfoCompleteDelegate::CreateWeakLambda(this, [this, OnComplete](int32 LocalUserNum, bool bWasSuccessful, const TArray<FUniqueNetIdRef>& UserIds, const FString& Error)
            {
                UserInterface->ClearOnQueryUserInfoCompleteDelegate_Handle(LocalUserNum, OnQueryUserInfoCompleteDelegateHandle);

                // Refresh friends data with queried friend's user information.
                TArray<TSharedRef<FOnlineFriend>> NewCachedFriendList;
                FriendsInterface->GetFriendsList(LocalUserNum, TEXT(""), NewCachedFriendList);
                for (const TSharedRef<FOnlineFriend>& NewCachedFriend : NewCachedFriendList)
                {
                    // Update friend's avatar URL based on queried friend's user information.
                    FString UserAvatarURL;
                    TSharedPtr<FOnlineUser> UserInfo = UserInterface->GetUserInfo(LocalUserNum, NewCachedFriend.Get().GetUserId().Get());
                    UserInfo->GetUserAttribute(ACCELBYTE_ACCOUNT_GAME_AVATAR_URL, UserAvatarURL);
                    StaticCastSharedRef<FOnlineFriendAccelByte>(NewCachedFriend).Get().SetUserAttribute(ACCELBYTE_ACCOUNT_GAME_AVATAR_URL, UserAvatarURL);
                }

                OnComplete.ExecuteIfBound(true, NewCachedFriendList, TEXT(""));
            }
        ));

        // Query friends' user information.
        UserInterface->QueryUserInfo(LocalUserNum, FriendIds);
    }
    // If none, request to backend then get the cached the friend list.
    else
    {
        FriendsInterface->ReadFriendsList(
            LocalUserNum, 
            TEXT(""), 
            FOnReadFriendsListComplete::CreateWeakLambda(this, [this, OnComplete](int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& Error)
            {
                TArray<TSharedRef<FOnlineFriend>> CachedFriendList;
                FriendsInterface->GetFriendsList(LocalUserNum, TEXT(""), CachedFriendList);

                OnComplete.ExecuteIfBound(bWasSuccessful, CachedFriendList, Error);
            }
        ));
    }
}

void UFriendsSubsystem::GetSelfFriendCode(const APlayerController* PC, const FOnGetSelfFriendCodeComplete& OnComplete)
{
    if (!ensure(UserInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot get self friend code. User Interface is not valid."));
        return;
    }

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    const FUniqueNetIdPtr LocalPlayerId = GetUniqueNetIdFromPlayerController(PC);
    if (!ensure(LocalPlayerId.IsValid()))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot get self friend code. LocalPlayer NetId is not valid."));
        return;
    }

    // Try to get friend code from cache.
    if (const TSharedPtr<FOnlineUser> UserInfo = UserInterface->GetUserInfo(LocalUserNum, LocalPlayerId.ToSharedRef().Get()))
    {
        if (const TSharedPtr<FUserOnlineAccountAccelByte> UserAccount = StaticCastSharedPtr<FUserOnlineAccountAccelByte>(UserInfo))
        {
            const FString FriendCode = UserAccount->GetPublicCode();
            UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to get self friend code: %s"), *FriendCode);
            OnComplete.ExecuteIfBound(true, UFriendData::ConvertToFriendData(UserInfo.ToSharedRef()), FriendCode);
            return;
        }
    }

    // If not available on cache then query the user info to get friend code.
    OnQueryUserToGetFriendCodeDelegateHandle = UserInterface->AddOnQueryUserInfoCompleteDelegate_Handle(
        LocalUserNum,
        FOnQueryUserInfoCompleteDelegate::CreateWeakLambda(this, [this, OnComplete](int32 LocalUserNum, bool bWasSuccessful, const TArray<FUniqueNetIdRef>& UserIds, const FString& ErrorStr)
        {
            UserInterface->ClearOnQueryUserInfoCompleteDelegate_Handle(LocalUserNum, OnQueryUserToGetFriendCodeDelegateHandle);

            if (!bWasSuccessful || UserIds.IsEmpty()) 
            {
                UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to get self friend code. Query user info is failed."));
                OnComplete.ExecuteIfBound(false, nullptr, TEXT(""));
                return;
            }

            const FUniqueNetIdRef UserId = UserIds[0];
            const TSharedPtr<FOnlineUser> UserInfo = UserInterface->GetUserInfo(LocalUserNum, UserId.Get());
            if (!UserInfo) 
            {
                UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to get self friend code. User info is not found."));
                OnComplete.ExecuteIfBound(false, nullptr, TEXT(""));
                return;
            }

            const TSharedPtr<FUserOnlineAccountAccelByte> UserAccount = StaticCastSharedPtr<FUserOnlineAccountAccelByte>(UserInfo);
            if (!UserAccount) 
            {
                UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to get self friend code. User account is not found."));
                OnComplete.ExecuteIfBound(false, nullptr, TEXT(""));
                return;
            }

            const FString FriendCode = UserAccount->GetPublicCode();
            UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to get self friend code: %s"), *FriendCode);
            OnComplete.ExecuteIfBound(true, UFriendData::ConvertToFriendData(UserInfo.ToSharedRef()), FriendCode);
        }
    ));
    UserInterface->QueryUserInfo(LocalUserNum, TPartyMemberArray{ LocalPlayerId.ToSharedRef() });
}

void UFriendsSubsystem::FindFriend(const APlayerController* PC, const FString& InKeyword, const FOnFindFriendComplete& OnComplete)
{
    if (!ensure(FriendsInterface) || !ensure(UserInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot find a friend. Friends Interface or User Interface is not valid."));
        return;
    }
    
    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    const FUniqueNetIdPtr LocalPlayerId = GetUniqueNetIdFromPlayerController(PC);
    if (!ensure(LocalPlayerId.IsValid()))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot find friends. LocalPlayer NetId is not valid."));
        return;
    }

    GetCacheFriendList(LocalUserNum, FOnGetCacheFriendListComplete::CreateWeakLambda(this, [this, LocalPlayerId, LocalUserNum, InKeyword, OnComplete](bool bWasSuccessful, TArray<TSharedRef<FOnlineFriend>>& CachedFriendList, const FString& ErrorMessage)
    {
        if (bWasSuccessful)
        {
            // Find friend by exact display name.
            UserInterface->QueryUserIdMapping(LocalPlayerId.ToSharedRef().Get(), InKeyword, IOnlineUser::FOnQueryUserMappingComplete::CreateUObject(this, &ThisClass::OnFindFriendComplete, LocalUserNum, OnComplete));
        }
        else
        {
            OnComplete.ExecuteIfBound(false, nullptr, ErrorMessage);
        }
    }));
}

void UFriendsSubsystem::OnFindFriendComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& DisplayName, const FUniqueNetId& FoundUserId, const FString& Error, int32 LocalUserNum, const FOnFindFriendComplete OnComplete)
{
    if (bWasSuccessful)
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to find a friend with keyword: %s"), *DisplayName);

        // Check if the found user is the player it self.
        if (UserId == FoundUserId) 
        {
            OnComplete.ExecuteIfBound(false, nullptr, CANNOT_INVITE_FRIEND_SELF.ToString());
            return;
        }

        // Check if the found user is already friend.
        TSharedPtr<FOnlineFriend> FoundFriend = FriendsInterface->GetFriend(LocalUserNum, FoundUserId, TEXT(""));
        if (FoundFriend.IsValid())
        {
            OnComplete.ExecuteIfBound(true, UFriendData::ConvertToFriendData(FoundFriend.ToSharedRef()), TEXT(""));
            return;
        }

        // Request the found user information to backend (to retrieve avatar URL, display name, etc.)
        OnQueryUserInfoCompleteDelegateHandle = UserInterface->AddOnQueryUserInfoCompleteDelegate_Handle(
            LocalUserNum,
            FOnQueryUserInfoCompleteDelegate::CreateWeakLambda(this, [this, OnComplete](int32 LocalUserNum, bool bWasSuccessful, const TArray<FUniqueNetIdRef>& UserIds, const FString& ErrorStr)
            {
                UserInterface->ClearOnQueryUserInfoCompleteDelegate_Handle(LocalUserNum, OnQueryUserInfoCompleteDelegateHandle);

                if (bWasSuccessful && !UserIds.IsEmpty())
                {
                    OnComplete.ExecuteIfBound(true, UFriendData::ConvertToFriendData(UserInterface->GetUserInfo(LocalUserNum, UserIds[0].Get()).ToSharedRef()), TEXT(""));
                }
                else
                {
                    OnComplete.ExecuteIfBound(false, nullptr, ErrorStr);
                }
            }
        ));
        UserInterface->QueryUserInfo(LocalUserNum, TPartyMemberArray{ FoundUserId.AsShared() });
    }
    else
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to find a friend with keyword: %s"), *DisplayName);
        OnComplete.ExecuteIfBound(false, nullptr, Error);
    }
}

void UFriendsSubsystem::SendFriendRequest(const APlayerController* PC, const FUniqueNetIdRepl FriendUserId, const FOnSendFriendRequestComplete& OnComplete)
{
    if (!ensure(FriendsInterface) || !ensure(PromptSubsystem))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot send friend request. Friends Interface or Prompt Subsystem is not valid."));
        return;
    }

    PromptSubsystem->ShowLoading(SEND_FRIEND_REQUEST_MESSAGE);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);

    // Send friend requests by friend's user id. We leave the ListName argument empty since the AccelByte OSS does not require it.
    FriendsInterface->SendInvite(LocalUserNum, *FriendUserId.GetUniqueNetId().Get(), TEXT(""), FOnSendInviteComplete::CreateUObject(this, &ThisClass::OnSendFriendRequestComplete, OnComplete));
}

void UFriendsSubsystem::SendFriendRequest(const APlayerController* PC, const FString& FriendCode, const FOnSendFriendRequestComplete& OnComplete)
{
    if (!ensure(FriendsInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot send friend request by friend code. Friends Interface or Prompt Subsystem is not valid."));
        return;
    }

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);

    // Send friend requests by friend code. We leave the ListName argument empty since the AccelByte OSS does not require it.
    FriendsInterface->SendInvite(LocalUserNum, FriendCode, TEXT(""), FOnSendInviteComplete::CreateUObject(this, &ThisClass::OnSendFriendRequestByFriendCodeComplete, OnComplete));
}

void UFriendsSubsystem::OnSendFriendRequestComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr, const FOnSendFriendRequestComplete OnComplete)
{
    PromptSubsystem->HideLoading();

    TSharedPtr<FOnlineFriend> FoundFriend = FriendsInterface->GetFriend(LocalUserNum, FriendId, TEXT(""));
    if (bWasSuccessful && FoundFriend.IsValid())
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to send a friend request."));

        PromptSubsystem->ShowMessagePopUp(MESSAGE_PROMPT_TEXT, SUCCESS_SEND_FRIEND_REQUEST);
        OnComplete.ExecuteIfBound(true, UFriendData::ConvertToFriendData(FoundFriend.ToSharedRef()), TEXT(""));
    }
    else
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to send a friend request. Error: %s"), *ErrorStr);

        PromptSubsystem->ShowMessagePopUp(ERROR_PROMPT_TEXT, FText::FromString(ErrorStr));
        OnComplete.ExecuteIfBound(false, nullptr, ErrorStr);
    }
}

void UFriendsSubsystem::OnSendFriendRequestByFriendCodeComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr, const FOnSendFriendRequestComplete OnComplete)
{
    // Get cached friend info to retrieve the updated friend data to be returned to the callback.
    FUniqueNetIdPtr FriendUserId = FriendId.AsShared();
    GetCacheFriendList(
        LocalUserNum,
        FOnGetCacheFriendListComplete::CreateWeakLambda(this, [this, LocalUserNum, bWasSuccessful, FriendUserId, ErrorStr, OnComplete]
        (bool bQueryWasSuccessful, TArray<TSharedRef<FOnlineFriend>>& CachedFriendList, const FString& ErrorMessage)
        {
            UFriendData* FriendData = nullptr;
            if (FriendUserId) 
            {
                TSharedPtr<FOnlineFriend> FoundFriend = FriendsInterface->GetFriend(LocalUserNum, FriendUserId.ToSharedRef().Get(), TEXT(""));
                if (FoundFriend)
                {
                    FriendData = UFriendData::ConvertToFriendData(FoundFriend.ToSharedRef());
                }
            }

            if (bWasSuccessful)
            {
                UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to send a friend request by friend code."));

                PromptSubsystem->ShowMessagePopUp(MESSAGE_PROMPT_TEXT, SUCCESS_SEND_FRIEND_REQUEST_BY_FRIEND_CODE);
                OnComplete.ExecuteIfBound(true, FriendData, TEXT(""));
            }
            else
            {
                UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to send a friend request by friend code. Error: %s"), *ErrorStr);
                OnComplete.ExecuteIfBound(false, FriendData, ErrorStr);
            }
        }
    ));
}

#pragma endregion


#pragma region Module.8b Function Definitions

void UFriendsSubsystem::BindOnCachedFriendsDataUpdated(const APlayerController* PC, const FOnCachedFriendsDataUpdated& Delegate)
{
    ensure(FriendsInterface);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);

    // Add on friends changed delegate.
    OnFriendsChangeDelegateHandles.Add(LocalUserNum, FriendsInterface->AddOnFriendsChangeDelegate_Handle(LocalUserNum, FOnFriendsChangeDelegate::CreateWeakLambda(this, [Delegate]() { Delegate.ExecuteIfBound(); })));
}

void UFriendsSubsystem::UnbindOnCachedFriendsDataUpdated(const APlayerController* PC)
{
    ensure(FriendsInterface);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);

    // Clear on friends changed delegate.
    FDelegateHandle TempHandle = OnFriendsChangeDelegateHandles[LocalUserNum];
    if (TempHandle.IsValid())
    {
        FriendsInterface->ClearOnFriendsChangeDelegate_Handle(LocalUserNum, TempHandle);
    }
}

void UFriendsSubsystem::GetInboundFriendRequestList(const APlayerController* PC, const FOnGetInboundFriendRequestListComplete& OnComplete)
{
    if (!ensure(FriendsInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot query friend request list. Friends Interface is not valid."));
        return;
    }

    // Get friend inbound request list from cache.
    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    GetCacheFriendList(LocalUserNum, FOnGetCacheFriendListComplete::CreateWeakLambda(this, [this, OnComplete](bool bWasSuccessful, TArray<TSharedRef<FOnlineFriend>>& CachedFriendList, const FString& ErrorMessage)
    {
        if (bWasSuccessful)
        {
            // Filter pending inbound friend requests.
            CachedFriendList = CachedFriendList.FilterByPredicate([](const TSharedRef<FOnlineFriend>& Friend)
            {
                return Friend->GetInviteStatus() == EInviteStatus::PendingInbound;
            });

            TArray<UFriendData*> InboundFriendRequestList;
            for (const TSharedRef<FOnlineFriend>& TempData : CachedFriendList)
            {
                InboundFriendRequestList.Add(UFriendData::ConvertToFriendData(TempData));
            }

            OnComplete.ExecuteIfBound(true, InboundFriendRequestList, TEXT(""));
        }
        else
        {
            OnComplete.ExecuteIfBound(false, TArray<UFriendData*>(), ErrorMessage);
        }
    }));
}

void UFriendsSubsystem::GetOutboundFriendRequestList(const APlayerController* PC, const FOnGetOutboundFriendRequestListComplete& OnComplete)
{
    if (!ensure(FriendsInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot query friend request list. Friends Interface is not valid."));
        return;
    }

    // Get friend outbound request list from cache.
    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    GetCacheFriendList(LocalUserNum, FOnGetCacheFriendListComplete::CreateWeakLambda(this, [this, OnComplete](bool bWasSuccessful, TArray<TSharedRef<FOnlineFriend>>& CachedFriendList, const FString& ErrorMessage)
    {
        if (bWasSuccessful)
        {
            // Filter pending outbound friend requests.
            CachedFriendList = CachedFriendList.FilterByPredicate([](const TSharedRef<FOnlineFriend>& Friend)
            {
                return Friend->GetInviteStatus() == EInviteStatus::PendingOutbound;
            });

            TArray<UFriendData*> OutbondFriendRequestList;
            for (const TSharedRef<FOnlineFriend>& TempData : CachedFriendList)
            {
                OutbondFriendRequestList.Add(UFriendData::ConvertToFriendData(TempData));
            }

            OnComplete.ExecuteIfBound(true, OutbondFriendRequestList, TEXT(""));
        }
        else
        {
            OnComplete.ExecuteIfBound(false, TArray<UFriendData*>(), ErrorMessage);
        }
    }));
}

void UFriendsSubsystem::AcceptFriendRequest(const APlayerController* PC, const FUniqueNetIdRepl FriendUserId, const FOnAcceptFriendRequestComplete& OnComplete)
{
    if (!ensure(FriendsInterface) || !ensure(PromptSubsystem))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot accept friend request. Friends Interface or Prompt Subsystem is not valid."));
        return;
    }

    PromptSubsystem->ShowLoading(ACCEPT_FRIEND_REQUEST_MESSAGE);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    FriendsInterface->AcceptInvite(LocalUserNum, FriendUserId.GetUniqueNetId().ToSharedRef().Get(), TEXT(""), FOnAcceptInviteComplete::CreateUObject(this, &ThisClass::OnAcceptFriendRequestComplete, OnComplete));
}

void UFriendsSubsystem::OnAcceptFriendRequestComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr, const FOnAcceptFriendRequestComplete OnComplete)
{
    PromptSubsystem->HideLoading();

    if (bWasSuccessful)
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to accept a friend request."));
        
        PromptSubsystem->ShowMessagePopUp(MESSAGE_PROMPT_TEXT, SUCCESS_ACCEPT_FRIEND_REQUEST);
        OnComplete.ExecuteIfBound(true, TEXT(""));
    }
    else
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to accept a friend request. Error: %s"), *ErrorStr);

        PromptSubsystem->ShowMessagePopUp(ERROR_PROMPT_TEXT, FText::FromString(ErrorStr));
        OnComplete.ExecuteIfBound(false, ErrorStr);
    }
}

void UFriendsSubsystem::RejectFriendRequest(const APlayerController* PC, const FUniqueNetIdRepl FriendUserId, const FOnRejectFriendRequestComplete& OnComplete)
{
    if (!ensure(FriendsInterface) || !ensure(PromptSubsystem))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot reject friend request. Friends Interface or Prompt Subsystem is not valid."));
        return;
    }

    PromptSubsystem->ShowLoading(REJECT_FRIEND_REQUEST_MESSAGE);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    OnRejectFriendRequestCompleteDelegateHandle = FriendsInterface->AddOnRejectInviteCompleteDelegate_Handle(LocalUserNum, FOnRejectInviteCompleteDelegate::CreateUObject(this, &ThisClass::OnRejectFriendRequestComplete, OnComplete));
    FriendsInterface->RejectInvite(LocalUserNum, FriendUserId.GetUniqueNetId().ToSharedRef().Get(), TEXT(""));
}

void UFriendsSubsystem::OnRejectFriendRequestComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr, const FOnRejectFriendRequestComplete OnComplete)
{
    PromptSubsystem->HideLoading();

    FriendsInterface->ClearOnRejectInviteCompleteDelegate_Handle(LocalUserNum, OnRejectFriendRequestCompleteDelegateHandle);

    if (bWasSuccessful)
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to reject a friend request."));

        PromptSubsystem->ShowMessagePopUp(MESSAGE_PROMPT_TEXT, SUCCESS_REJECT_FRIEND_REQUEST);
        OnComplete.ExecuteIfBound(true, TEXT(""));
    }
    else
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to reject a friend request. Error: %s"), *ErrorStr);

        PromptSubsystem->ShowMessagePopUp(ERROR_PROMPT_TEXT, FText::FromString(ErrorStr));
        OnComplete.ExecuteIfBound(false, ErrorStr);
    }
}

void UFriendsSubsystem::CancelFriendRequest(const APlayerController* PC, const FUniqueNetIdRepl FriendUserId, const FOnCancelFriendRequestComplete& OnComplete)
{
    if (!ensure(FriendsInterface) || !ensure(PromptSubsystem))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot cancel friend request. Friends Interface or Prompt Subsystem is not valid."));
        return;
    }

    PromptSubsystem->ShowLoading(CANCEL_FRIEND_REQUEST_MESSAGE);

    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    OnCancelFriendRequestCompleteDelegateHandle = FriendsInterface->AddOnDeleteFriendCompleteDelegate_Handle(LocalUserNum, FOnDeleteFriendCompleteDelegate::CreateUObject(this, &ThisClass::OnCancelFriendRequestComplete, OnComplete));
    FriendsInterface->DeleteFriend(LocalUserNum, FriendUserId.GetUniqueNetId().ToSharedRef().Get(), TEXT(""));
}

void UFriendsSubsystem::OnCancelFriendRequestComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr, const FOnCancelFriendRequestComplete OnComplete)
{
    PromptSubsystem->HideLoading();

    FriendsInterface->ClearOnDeleteFriendCompleteDelegate_Handle(LocalUserNum, OnCancelFriendRequestCompleteDelegateHandle);

    if (bWasSuccessful)
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Success to cancel a friend request."));

        PromptSubsystem->ShowMessagePopUp(MESSAGE_PROMPT_TEXT, SUCCESS_CANCEL_FRIEND_REQUEST);
        OnComplete.ExecuteIfBound(true, TEXT(""));
    }
    else
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Failed to cancel a friend request. Error: %s"), *ErrorStr);

        PromptSubsystem->ShowMessagePopUp(ERROR_PROMPT_TEXT, FText::FromString(ErrorStr));
        OnComplete.ExecuteIfBound(false, ErrorStr);
    }
}

#pragma endregion


#pragma region Module.8c Function Definitions

void UFriendsSubsystem::GetFriendList(const APlayerController* PC, const FOnGetFriendListComplete& OnComplete)
{
    if (!ensure(FriendsInterface))
    {
        UE_LOG_FRIENDS_ESSENTIALS(Warning, TEXT("Cannot get friend list. Friends Interface is not valid."));
        return;
    }

    // Get accepted friend list from cache.
    const int32 LocalUserNum = GetLocalUserNumFromPlayerController(PC);
    GetCacheFriendList(LocalUserNum, FOnGetCacheFriendListComplete::CreateWeakLambda(this, [this, OnComplete](bool bWasSuccessful, TArray<TSharedRef<FOnlineFriend>>& CachedFriendList, const FString& ErrorMessage)
    {
        if (bWasSuccessful)
        {
            // Filter accepted friends.
            CachedFriendList = CachedFriendList.FilterByPredicate([](const TSharedRef<FOnlineFriend>& Friend)
            {
                return Friend->GetInviteStatus() == EInviteStatus::Accepted;
            });

            TArray<UFriendData*> AcceptedFriendList;
            for (const TSharedRef<FOnlineFriend>& TempData : CachedFriendList)
            {
                AcceptedFriendList.Add(UFriendData::ConvertToFriendData(TempData));
            }

            OnComplete.ExecuteIfBound(true, AcceptedFriendList, TEXT(""));
        }
        else
        {
            OnComplete.ExecuteIfBound(false, TArray<UFriendData*>(), ErrorMessage);
        }
    }));
}

#pragma endregion


#undef LOCTEXT_NAMESPACE