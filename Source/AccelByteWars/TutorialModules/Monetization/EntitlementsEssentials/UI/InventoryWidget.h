﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "Core/System/AccelByteWarsGameInstance.h"

#include "CoreMinimal.h"
#include "OnlineError.h"
#include "Core/UI/AccelByteWarsActivatableWidget.h"
#include "Core/UI/MainMenu/Store/StoreItemModel.h"
#include "InventoryWidget.generated.h"

class UCommonButtonBase;
class UAccelByteWarsAsyncImageWidget;
class UTextBlock;
class UTileView;
class UEntitlementsEssentialsSubsystem;
class UAccelByteWarsWidgetSwitcher;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryMenuActivated, const APlayerController* /*PlayerController*/, const TDelegate<void()> /*Callback*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryMenuDeactivated, const APlayerController* /*PlayerController*/, const TDelegate<void()> /*Callback*/);

UCLASS(Abstract)
class ACCELBYTEWARS_API UInventoryWidget : public UAccelByteWarsActivatableWidget
{
	GENERATED_BODY()

public:
	inline static FOnInventoryMenuActivated OnInventoryMenuActivated;
	inline static FOnInventoryMenuDeactivated OnInventoryMenuDeactivated;

	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	UPROPERTY()
	UEntitlementsEssentialsSubsystem* EntitlementsSubsystem;

	UPROPERTY(EditAnywhere)
	bool bIsConsumable = true;

	void ShowEntitlements(const FOnlineError& Error, const TArray<UItemDataObject*> Entitlements) const;

#pragma region "UI Related"
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void OnClickListItem(UObject* Object);
	void OnClickEquip();
	void OnClickUnequip();

	UPROPERTY()
	UItemDataObject* SelectedItem;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UAccelByteWarsWidgetSwitcher* Ws_Root;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UAccelByteWarsAsyncImageWidget* W_SelectedItem_Preview;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UTextBlock* Tb_SelectedItem_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UTileView* Tv_Content;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UCommonButtonBase* Btn_Equip;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UCommonButtonBase* Btn_Unequip;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	UCommonButtonBase* Btn_Back;
#pragma endregion 
};
