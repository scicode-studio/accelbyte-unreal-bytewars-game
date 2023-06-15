// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "TutorialModules/Module-8/UI/FriendsWidget_Starter.h"
#include "TutorialModules/Module-8/UI/FriendDetailsWidget_Starter.h"
#include "Core/System/AccelByteWarsGameInstance.h"
#include "Core/UI/Components/AccelByteWarsWidgetList.h"
#include "Core/UI/AccelByteWarsBaseUI.h"
#include "CommonButtonBase.h"

void UFriendsWidget_Starter::NativeConstruct()
{
	Super::NativeConstruct();

	GameInstance = Cast<UAccelByteWarsGameInstance>(GetGameInstance());
	ensure(GameInstance);

	FriendsSubsystem = GameInstance->GetSubsystem<UFriendsSubsystem_Starter>();
	ensure(FriendsSubsystem);
}

void UFriendsWidget_Starter::NativeOnActivated()
{
	Super::NativeOnActivated();

	WidgetList->GetListView()->OnItemClicked().AddUObject(this, &ThisClass::OnFriendEntryClicked);

	// TODO: Bind event to refresh friend list here.

	GetFriendList();
}

void UFriendsWidget_Starter::NativeOnDeactivated()
{
	WidgetList->GetListView()->OnItemClicked().Clear();

	// TODO: Unbind event to refresh friend list here.

	Super::NativeOnDeactivated();
}

void UFriendsWidget_Starter::GetFriendList()
{
	// TODO: Get and display friend list here.
}

void UFriendsWidget_Starter::OnFriendEntryClicked(UObject* Item)
{
	UFriendData* FriendData = Cast<UFriendData>(Item);
	ensure(FriendData);

	UAccelByteWarsBaseUI* BaseUIWidget = Cast<UAccelByteWarsBaseUI>(GameInstance->GetBaseUIWidget());
	ensure(BaseUIWidget);

	UFriendDetailsWidget_Starter* DetailsWidget = Cast<UFriendDetailsWidget_Starter>(BaseUIWidget->PushWidgetToStack(EBaseUIStackType::Menu, FriendDetailsWidgetClass));
	ensure(DetailsWidget);

	DetailsWidget->InitData(FriendData);
}