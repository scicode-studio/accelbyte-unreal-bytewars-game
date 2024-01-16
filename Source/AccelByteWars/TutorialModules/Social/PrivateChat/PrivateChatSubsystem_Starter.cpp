﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "PrivateChatSubsystem_Starter.h"
#include "OnlineSubsystemUtils.h"

#include "Core/System/AccelByteWarsGameInstance.h"
#include "Core/UI/AccelByteWarsBaseUI.h"
#include "Core/UI/Components/Prompt/PromptSubsystem.h"
#include "Social/FriendsEssentials/UI/FriendDetailsWidget.h"
#include "Social/PrivateChat/UI/PrivateChatWidget_Starter.h"

#include "TutorialModuleUtilities/TutorialModuleOnlineUtility.h"

void UPrivateChatSubsystem_Starter::Initialize(FSubsystemCollectionBase& Collection)
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

            if (FriendUserId == nullptr || !FriendUserId.IsValid())
            {
                return;
            }

            if (UPrivateChatWidget_Starter* PrivateChatWidget = Cast<UPrivateChatWidget_Starter>(BaseUIWidget->PushWidgetToStack(EBaseUIStackType::Menu, WidgetClass.Get())))
            {
                PrivateChatWidget->SetPrivateChatRecipient(FriendUserId.GetUniqueNetId());
            }
        });
    }

    // TODO: Bind private chat events here.
}

void UPrivateChatSubsystem_Starter::Deinitialize()
{
    Super::Deinitialize();

    // TODO: Unbind private chat events here.
}

void UPrivateChatSubsystem_Starter::PushPrivateChatMessageReceivedNotification(const FUniqueNetId& UserId, const TSharedRef<FChatMessage>& Message)
{
    if (!GetPromptSubsystem())
    {
        return;
    }

    // Only push a notification only if the player is not in the chat menu of the same recipient.
    const UCommonActivatableWidget* ActiveWidget = UAccelByteWarsBaseUI::GetActiveWidgetOfStack(EBaseUIStackType::Menu, this);
    if (const UPrivateChatWidget_Starter* PrivateChatWidget = Cast<UPrivateChatWidget_Starter>(ActiveWidget))
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

FOnlineChatAccelBytePtr UPrivateChatSubsystem_Starter::GetChatInterface() const
{
    const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (!ensure(Subsystem))
    {
        UE_LOG_PRIVATECHAT(Warning, TEXT("The online subsystem is invalid. Please make sure OnlineSubsystemAccelByte is enabled and DefaultPlatformService under [OnlineSubsystem] in the Engine.ini set to AccelByte."));
        return nullptr;
    }

    return StaticCastSharedPtr<FOnlineChatAccelByte>(Subsystem->GetChatInterface());
}

UPromptSubsystem* UPrivateChatSubsystem_Starter::GetPromptSubsystem() const
{
    const UAccelByteWarsGameInstance* GameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance());
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UPromptSubsystem>();
}

#pragma region Module Private Chat Function Definitions

// TODO: Add your Module Private Chat function definitions here.

#pragma endregion
