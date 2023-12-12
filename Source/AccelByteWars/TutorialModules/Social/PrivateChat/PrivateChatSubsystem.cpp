// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "PrivateChatSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "Core/System/AccelByteWarsGameInstance.h"
#include "Core/UI/AccelByteWarsBaseUI.h"
#include "Core/UI/Components/Prompt/PromptSubsystem.h"
#include "Social/FriendsEssentials/UI/FriendDetailsWidget.h"
#include "Social/FriendsEssentials/UI/FriendDetailsWidget_Starter.h"
#include "Social/PrivateChat/UI/PrivateChatWidget.h"

#include "TutorialModuleUtilities/TutorialModuleOnlineUtility.h"

void UPrivateChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Assign action button to open private chat.
    FTutorialModuleGeneratedWidget* PrivateChatButtonMetadata = FTutorialModuleGeneratedWidget::GetMetadataById(TEXT("btn_private_chat"));
    if (PrivateChatButtonMetadata)
    {
        if (!PrivateChatButtonMetadata->OwnerTutorialModule)
        {
            return;
        }

        // Get chat widget class.
        auto WidgetClass = PrivateChatButtonMetadata->OwnerTutorialModule->GetTutorialModuleUIClass();
        if (!WidgetClass)
        {
            return;
        }

        // Open private chat widget and inject the friend user id to it.
        PrivateChatButtonMetadata->ButtonAction.Clear();
        PrivateChatButtonMetadata->ButtonAction.AddWeakLambda(this, [this, WidgetClass]()
        {
            UAccelByteWarsGameInstance* GameInstance = StaticCast<UAccelByteWarsGameInstance*>(GetGameInstance());
            if (!GameInstance)
            {
                return;
            }

            UAccelByteWarsBaseUI* BaseUIWidget = GameInstance->GetBaseUIWidget();
            if (!BaseUIWidget)
            {
                return;
            }

            UCommonActivatableWidget* ParentWidget = UAccelByteWarsBaseUI::GetActiveWidgetOfStack(EBaseUIStackType::Menu, this);
            if (!ParentWidget)
            {
                return;
            }

            FUniqueNetIdRepl FriendUserId = nullptr;
            if (const UFriendDetailsWidget* FriendDetailsWidget = Cast<UFriendDetailsWidget>(ParentWidget))
            {
                if (FriendDetailsWidget->GetCachedFriendData() &&
                    FriendDetailsWidget->GetCachedFriendData()->UserId &&
                    FriendDetailsWidget->GetCachedFriendData()->UserId.IsValid())
                {
                    FriendUserId = FriendDetailsWidget->GetCachedFriendData()->UserId;
                }
            }
            else if (const UFriendDetailsWidget_Starter* FriendDetailsWidget_Starter = Cast<UFriendDetailsWidget_Starter>(ParentWidget))
            {
                if (FriendDetailsWidget_Starter->GetCachedFriendData() &&
                    FriendDetailsWidget_Starter->GetCachedFriendData()->UserId &&
                    FriendDetailsWidget_Starter->GetCachedFriendData()->UserId.IsValid())
                {
                    FriendUserId = FriendDetailsWidget_Starter->GetCachedFriendData()->UserId;
                }
            }

            if (FriendUserId == nullptr || !FriendUserId.IsValid())
            {
                return;
            }

            if (UPrivateChatWidget* PrivateChatWidget = Cast<UPrivateChatWidget>(BaseUIWidget->PushWidgetToStack(EBaseUIStackType::Menu, WidgetClass.Get())))
            {
                PrivateChatWidget->SetPrivateChatRecipient(FriendUserId.GetUniqueNetId());
            }
        });
    }

    if (GetChatInterface())
    {
        GetChatInterface()->OnSendChatCompleteDelegates.AddUObject(this, &ThisClass::OnSendPrivateChatComplete);
        GetChatInterface()->OnChatPrivateMessageReceivedDelegates.AddUObject(this, &ThisClass::OnPrivateChatMessageReceived);

        // Push a notification when received chat messages.
        GetChatInterface()->OnChatPrivateMessageReceivedDelegates.AddUObject(this, &ThisClass::PushPrivateChatMessageReceivedNotification);
    }
}

void UPrivateChatSubsystem::Deinitialize()
{
    Super::Deinitialize();

    if (GetChatInterface())
    {
        GetChatInterface()->OnSendChatCompleteDelegates.RemoveAll(this);
        GetChatInterface()->OnChatPrivateMessageReceivedDelegates.RemoveAll(this);
    }
}

FString UPrivateChatSubsystem::GetPrivateChatRoomId(const FUniqueNetIdPtr SenderUserId, const FUniqueNetIdPtr RecipientUserId)
{
    if (!GetChatInterface())
    {
        return FString();
    }

    if (!SenderUserId || !RecipientUserId)
    {
        return FString();
    }

    const FUniqueNetIdAccelByteUserPtr SenderABId = StaticCastSharedPtr<const FUniqueNetIdAccelByteUser>(SenderUserId);
    const FUniqueNetIdAccelByteUserPtr RecipientABId = StaticCastSharedPtr<const FUniqueNetIdAccelByteUser>(RecipientUserId);
    if (!SenderABId || !RecipientABId)
    {
        return FString();
    }

    return GetChatInterface()->PersonalChatTopicId(SenderABId->GetAccelByteId(), RecipientABId->GetAccelByteId());
}

void UPrivateChatSubsystem::SendPrivateChatMessage(const FUniqueNetIdPtr UserId, const FUniqueNetIdPtr RecipientUserId, const FString& Message)
{
    if (!GetChatInterface())
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot send private chat message. Chat Interface is not valid."));
        OnSendPrivateChatComplete(FString(), Message, FString(), false);
        return;
    }

    if (!UserId)
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot send private chat message. User NetId is not valid."));
        OnSendPrivateChatComplete(FString(), Message, FString(), false);
        return;
    }

    if (!RecipientUserId)
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot send private chat message. Recipient NetId is not valid."));
        OnSendPrivateChatComplete(FString(), Message, FString(), false);
        return;
    }

    GetChatInterface()->SendPrivateChat(UserId.ToSharedRef().Get(), RecipientUserId.ToSharedRef().Get(), Message);
}

bool UPrivateChatSubsystem::GetLastPrivateChatMessages(const FUniqueNetIdPtr UserId, const FChatRoomId& RoomId, const int32 NumMessages, TArray<TSharedRef<FChatMessage>>& OutMessages)
{
    if (!GetChatInterface())
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot get last chat messages. Chat Interface is not valid."));
        return false;
    }

    if (!UserId)
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot get last chat messages. User NetId is not valid."));
        return false;
    }

    // Abort if not a private chat room.
    if (RoomId.IsEmpty() || GetChatInterface()->GetChatRoomType(RoomId) != EAccelByteChatRoomType::PERSONAL)
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot get last chat messages. Room Id is empty or not a private chat room."));
        return false;
    }

    GetChatInterface()->GetLastMessages(UserId.ToSharedRef().Get(), RoomId, NumMessages, OutMessages);
    UE_LOG_PRIVATECHAT(Log, TEXT("Success to get last chat messages. Number of returned messages: %d"), OutMessages.Num());

    return true;
}

bool UPrivateChatSubsystem::IsMessageFromLocalUser(const FUniqueNetIdPtr UserId, const FChatMessage& Message)
{
    if (!GetChatInterface())
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot check whether chat message is from local user or not. Chat Interface is not valid."));
        return false;
    }

    if (!UserId)
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Cannot check whether chat message is from local user or not. User NetId is not valid."));
        return false;
    }

    return GetChatInterface()->IsMessageFromLocalUser(UserId.ToSharedRef().Get(), Message, true);
}

void UPrivateChatSubsystem::OnSendPrivateChatComplete(FString UserId, FString MsgBody, FString RoomId, bool bWasSuccessful)
{
    // Abort if the room id is not a private chat room.
    if (!GetChatInterface() || GetChatInterface()->GetChatRoomType(RoomId) != EAccelByteChatRoomType::PERSONAL)
    {
        return;
    }

    if (bWasSuccessful)
    {
        UE_LOG_PRIVATECHAT(Log, TEXT("Success to send chat message on Room %s"), *RoomId);
    }
    else
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("Failed to send chat message on Room %s"), *RoomId);
    }

    OnSendPrivateChatCompleteDelegates.Broadcast(UserId, MsgBody, RoomId, bWasSuccessful);
}

void UPrivateChatSubsystem::OnPrivateChatMessageReceived(const FUniqueNetId& UserId, const TSharedRef<FChatMessage>& Message)
{
    UE_LOG_PRIVATECHAT(Log,
        TEXT("Received private chat message from %s: %s"),
        !Message.Get().GetNickname().IsEmpty() ? *Message.Get().GetNickname() : *UTutorialModuleOnlineUtility::GetUserDefaultDisplayName(Message->GetUserId().Get()),
        *Message.Get().GetBody());

    OnPrivateChatMessageReceivedDelegates.Broadcast(UserId, Message);
}

void UPrivateChatSubsystem::PushPrivateChatMessageReceivedNotification(const FUniqueNetId& UserId, const TSharedRef<FChatMessage>& Message)
{
    if (!GetPromptSubsystem())
    {
        return;
    }

    // Only push a notification only if the player is not in the chat menu of the same recipient.
    const UCommonActivatableWidget* ActiveWidget = UAccelByteWarsBaseUI::GetActiveWidgetOfStack(EBaseUIStackType::Menu, this);
    if (const UPrivateChatWidget* PrivateChatWidget = Cast<UPrivateChatWidget>(ActiveWidget))
    {
        const FUniqueNetIdAccelByteUserPtr CurrentRecipientABId = StaticCastSharedPtr<const FUniqueNetIdAccelByteUser>(PrivateChatWidget->GetPrivateChatRecipient());
        const FUniqueNetIdAccelByteUserRef SenderABId = StaticCastSharedRef<const FUniqueNetIdAccelByteUser>(Message->GetUserId());

        if (!CurrentRecipientABId || !SenderABId.Get().IsValid())
        {
            return;
        }
        
        if (CurrentRecipientABId->GetAccelByteId().Equals(SenderABId->GetAccelByteId()))
        {
            return;   
        }
    }

    FString SenderStr = Message.Get().GetNickname();
    if (SenderStr.IsEmpty()) 
    {
        SenderStr = UTutorialModuleOnlineUtility::GetUserDefaultDisplayName(Message->GetUserId().Get());
    }
    GetPromptSubsystem()->PushNotification(FText::Format(PRIVATE_CHAT_RECEIVED_MESSAGE, FText::FromString(SenderStr)));
}

FOnlineChatAccelBytePtr UPrivateChatSubsystem::GetChatInterface() const
{
    const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (!ensure(Subsystem))
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("The online subsystem is invalid. Please make sure OnlineSubsystemAccelByte is enabled and DefaultPlatformService under [OnlineSubsystem] in the Engine.ini set to AccelByte."));
        return nullptr;
    }

    return StaticCastSharedPtr<FOnlineChatAccelByte>(Subsystem->GetChatInterface());
}

UPromptSubsystem* UPrivateChatSubsystem::GetPromptSubsystem() const
{
    const UAccelByteWarsGameInstance* GameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance());
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UPromptSubsystem>();
}
