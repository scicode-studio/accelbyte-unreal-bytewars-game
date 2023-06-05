// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Core/UI/AccelByteWarsActivatableWidget.h"
#include "TutorialModules/Module-8/FriendsSubsystem_Starter.h"
#include "FindFriendsWidget_Starter.generated.h"

class UAccelByteWarsWidgetList;
class UEditableText;

UCLASS(Abstract)
class ACCELBYTEWARS_API UFindFriendsWidget_Starter : public UAccelByteWarsActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UFUNCTION()
	void OnSearchBarCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFriendsSubsystem_Starter* FriendsSubsystem;
	FDelegateHandle OnFriendListUpdatedDelegateHandle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, BlueprintProtected = true, AllowPrivateAccess = true))
	UEditableText* Edt_SearchBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, BlueprintProtected = true, AllowPrivateAccess = true))
	UAccelByteWarsWidgetList* WidgetList;
};