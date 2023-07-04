// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "Core/UI/Components/Prompt/PushNotification/PushNotificationWidgetEntry.h"
#include "Core/UI/Components/AccelByteWarsButtonBase.h"
#include "Core/UI/Components/AccelByteWarsAsyncImageWidget.h"
#include "Components/TextBlock.h"


void UPushNotificationWidgetEntry::NativeConstruct()
{
	Super::NativeConstruct();

	// Assign action buttons.
	ActionButtons.Add(Btn_Action1);
	ActionButtons.Add(Btn_Action2);
	ActionButtons.Add(Btn_Action3);
}

void UPushNotificationWidgetEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	Super::NativeOnListItemObjectSet(ListItemObject);

	Notification = Cast<UPushNotification>(ListItemObject);
	if (!ensure(Notification))
	{
		return;
	}

	// Set image icon.
	if (Notification->IconImageURL.IsEmpty()) 
	{
		W_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
	else 
	{
		W_Icon->LoadImage(Notification->IconImageURL);
		W_Icon->SetVisibility(ESlateVisibility::Visible);
	}
	
	// Set entry message.
	Tb_Message->SetText(Notification->Message);

	// Spawn action buttons.
	int32 ActionButtonIndex = 0;
	for (UAccelByteWarsButtonBase* ActionButton : ActionButtons)
	{
		ActionButton->SetVisibility(ESlateVisibility::Collapsed);
		ActionButton->OnClicked().Clear();
	}
	for (const FText& ActionButtonText : Notification->ActionButtonTexts) 
	{
		if (ActionButtonText.IsEmpty()) 
		{
			continue;
		}

		ActionButtons[ActionButtonIndex]->SetVisibility(ESlateVisibility::Visible);
		ActionButtons[ActionButtonIndex]->OnClicked().AddUObject(this, &ThisClass::SubmitActionResult, static_cast<EPushNotificationActionResult>(ActionButtonIndex));
		ActionButtonIndex++;
	}
}

void UPushNotificationWidgetEntry::SubmitActionResult(EPushNotificationActionResult ActionButtonResult)
{
	if (!ensure(Notification)) 
	{
		return;
	}

	Notification->ActionButtonCallback.ExecuteIfBound(ActionButtonResult);
	Notification->ActionButtonDynamicCallback.ExecuteIfBound(ActionButtonResult);
}
