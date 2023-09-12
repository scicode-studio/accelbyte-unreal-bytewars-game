// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "UObject/NoExportTypes.h"
#include "CoreMinimal.h"
#include "FTUEModels.generated.h"

class UAccelByteWarsActivatableWidget;
class UCommonUserWidget;
class UTutorialModuleDataAsset;

UENUM(BlueprintType)
enum class EFTUEDialogueHorizontalAnchor : uint8
{
    MIDDLE = 0 UMETA(DisplayName = "Middle"),
    LEFT UMETA(DisplayName = "Left"),
    RIGHT UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class EFTUEDialogueVerticalAnchor : uint8
{
    MIDDLE = 0 UMETA(DisplayName = "Middle"),
    TOP UMETA(DisplayName = "Top"),
    BOTTOM UMETA(DisplayName = "Bottom")
};

UENUM(BlueprintType)
enum class EFTUEDialogueButtonActionType : uint8
{
	HYPERLINK_BUTTON = 0 UMETA(DisplayName = "Hyperlink Button"),
    ACTION_BUTTON UMETA(DisplayName = "Custom Action Button")
};

UENUM(BlueprintType)
enum class FFTUEDialogueButtonType : uint8
{
    NO_BUTTON = 0 UMETA(DisplayName = "No Button"),
    ONE_BUTTON UMETA(DisplayName = "One Button"),
    TWO_BUTTONS UMETA(DisplayName = "Two Buttons")
};

USTRUCT(BlueprintType)
struct FFTUEDialogueButtonModel
{
    GENERATED_BODY()

    void Reset() 
    {
        ButtonActionType = EFTUEDialogueButtonActionType::HYPERLINK_BUTTON;
        ButtonText = FText::GetEmpty();
        TargetURL = FString("");
        ButtonActionDelegate.Unbind();
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The action type that should be executed by the button."))
    EFTUEDialogueButtonActionType ButtonActionType = EFTUEDialogueButtonActionType::HYPERLINK_BUTTON;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The text to be shown on the button."))
    FText ButtonText;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (
        EditCondition = "ButtonActionType==EFTUEDialogueButtonActionType::HYPERLINK_BUTTON", 
        EditConditionHides, Tooltip = "URL to be opened when the button is clicked."))
    FString TargetURL;

    // Action button to be executed if the button action type is a custom action button.
    TDelegate<void()> ButtonActionDelegate;
};

USTRUCT(BlueprintType)
struct FFTUEDialogueModel
{
    GENERATED_BODY()

    FVector2D GetAnchor() const
    {
        float Horizontal, Vertical;

        switch (HorizontalAnchor)
        {
        case EFTUEDialogueHorizontalAnchor::MIDDLE:
            Horizontal = 0.5f;
            break;
        case EFTUEDialogueHorizontalAnchor::LEFT:
            Horizontal = 0.0f;
            break;
        case EFTUEDialogueHorizontalAnchor::RIGHT:
            Horizontal = 1.0f;
            break;
        default:
            Horizontal = 0.5f;
            break;
        }

        switch (VerticalAnchor)
        {
        case EFTUEDialogueVerticalAnchor::MIDDLE:
            Vertical = 0.5f;
            break;
        case EFTUEDialogueVerticalAnchor::TOP:
            Vertical = 0.0f;
            break;
        case EFTUEDialogueVerticalAnchor::BOTTOM:
            Vertical = 1.0f;
            break;
        default:
            Vertical = 0.5f;
            break;
        }

        return FVector2D(Horizontal, Vertical);
    }
    
    bool operator<(const FFTUEDialogueModel& Other) const
    {
        return OrderPriority <= Other.OrderPriority;
    }

    UPROPERTY(VisibleAnywhere, 
        BlueprintReadOnly, meta = (
            Tooltip = "The Tutorial Module who owns the metadata of the FTUE.", 
            DisplayThumbnail = false))
    UTutorialModuleDataAsset* OwnerTutorialModule = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The message to be shown on the FTUE."))
    FText Message;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The type of action buttons should be shown when the FTUE is shown."))
    FFTUEDialogueButtonType ButtonType = FFTUEDialogueButtonType::NO_BUTTON;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        EditCondition = "ButtonType!=FFTUEDialogueButtonType::NO_BUTTON", 
        EditConditionHides))
    FFTUEDialogueButtonModel Button1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        EditCondition = "ButtonType==FFTUEDialogueButtonType::TWO_BUTTONS", 
        EditConditionHides))
    FFTUEDialogueButtonModel Button2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "Whether the FTUE should highlight a widget when it is shown."))
    bool bHighlightWidget = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        EditCondition = "bHighlightWidget", 
        EditConditionHides, 
        Tooltip = "The class type of the widget to be highlighted."))
    TSubclassOf<UCommonUserWidget> TargetWidgetClassToHighlight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        EditCondition = "bHighlightWidget", 
        EditConditionHides, 
        Tooltip = "The name of the widget to be highlighted."))
    FString TargetWidgetNameToHighlight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The vertical anchor of the FTUE relative to the screen."))
    EFTUEDialogueVerticalAnchor VerticalAnchor = EFTUEDialogueVerticalAnchor::MIDDLE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The horizontal anchor of the FTUE relative to the screen."))
    EFTUEDialogueHorizontalAnchor HorizontalAnchor = EFTUEDialogueHorizontalAnchor::MIDDLE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The position where the FTUE should be shown."))
    FVector2D Position = FVector2D(0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "The order when the FTUE should be shown."))
    int32 OrderPriority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "Whether the FTUE should lock player's accessibility to access other UIs when the FTUE is shown."))
    bool bIsInterrupting = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (
        Tooltip = "Target widget where the FTUE should be shown."))
    TArray<TSubclassOf<UAccelByteWarsActivatableWidget>> TargetWidgetClasses;

    // Flag to define whether this dialogue is already shown or not.
    bool bIsAlreadyShown = false;
};