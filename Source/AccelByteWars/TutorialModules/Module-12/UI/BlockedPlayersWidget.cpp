// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "TutorialModules/Module-12/UI/BlockedPlayersWidget.h"
#include "Core/UI/Components/AccelByteWarsWidgetSwitcher.h"
#include "Components/ListView.h"

void UBlockedPlayersWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ManagingFriendsSubsystem = GetGameInstance()->GetSubsystem<UManagingFriendsSubsystem>();
	ensure(ManagingFriendsSubsystem);
}

void UBlockedPlayersWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Reset widgets.
	Ws_BlockedPlayers->SetWidgetState(EAccelByteWarsWidgetSwitcherState::Empty);
	Lv_BlockedPlayers->ClearListItems();

	ManagingFriendsSubsystem->BindOnCachedBlockedPlayersDataUpdated(GetOwningPlayer(), FOnGetCacheBlockedPlayersDataUpdated::CreateUObject(this, &ThisClass::GetBlockedPlayerList));
	GetBlockedPlayerList();
}

void UBlockedPlayersWidget::NativeOnDeactivated()
{
	ManagingFriendsSubsystem->UnbindOnCachedBlockedPlayersDataUpdated(GetOwningPlayer());

	Super::NativeOnDeactivated();
}

void UBlockedPlayersWidget::GetBlockedPlayerList()
{
	ensure(ManagingFriendsSubsystem);

	Ws_BlockedPlayers->SetWidgetState(EAccelByteWarsWidgetSwitcherState::Loading);

	ManagingFriendsSubsystem->GetBlockedPlayerList(
		GetOwningPlayer(),
		FOnGetBlockedPlayerListComplete::CreateWeakLambda(this, [this](bool bWasSuccessful, TArray<UFriendData*> BlockedPlayers, const FString& ErrorMessage)
		{
			Lv_BlockedPlayers->SetUserFocus(GetOwningPlayer());
			Lv_BlockedPlayers->ClearListItems();

			if (bWasSuccessful)
			{
				Lv_BlockedPlayers->SetListItems(BlockedPlayers);
				Ws_BlockedPlayers->SetWidgetState(BlockedPlayers.IsEmpty() ?
					EAccelByteWarsWidgetSwitcherState::Empty :
					EAccelByteWarsWidgetSwitcherState::Not_Empty);
			}
			else
			{
				Ws_BlockedPlayers->ErrorMessage = FText::FromString(ErrorMessage);
				Ws_BlockedPlayers->SetWidgetState(EAccelByteWarsWidgetSwitcherState::Error);
			}
		}
	));
}