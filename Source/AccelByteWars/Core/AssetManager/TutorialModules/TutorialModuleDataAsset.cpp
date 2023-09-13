// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "Core/AssetManager/TutorialModules/TutorialModuleDataAsset.h"

#include "TutorialModuleOnlineSession.h"
#include "Core/AssetManager/TutorialModules/TutorialModuleSubsystem.h"
#include "Core/UI/AccelByteWarsActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

const FPrimaryAssetType	UTutorialModuleDataAsset::TutorialModuleAssetType = TEXT("TutorialModule");
TSet<FString> UTutorialModuleDataAsset::GeneratedWidgetUsedIds;
TSet<FString> UTutorialModuleDataAsset::FTUEDialogueUsedIds;

FTutorialModuleData UTutorialModuleDataAsset::GetTutorialModuleDataByCodeName(const FString& InCodeName)
{
	FPrimaryAssetId TutorialModuleAssetId = GenerateAssetIdFromCodeName(InCodeName);

	FTutorialModuleData TutorialModuleData;
	TutorialModuleData.CodeName = InCodeName;

	TutorialModuleData.DisplayName = UAccelByteWarsDataAsset::GetDisplayNameForAsset(TutorialModuleAssetId);
	TutorialModuleData.Description = UAccelByteWarsDataAsset::GetDescriptionForAsset(TutorialModuleAssetId);

	FString DefaultClassString = UAccelByteWarsDataAsset::GetMetadataForAsset<FString>(TutorialModuleAssetId, GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, DefaultUIClass));
	TSoftClassPtr<UAccelByteWarsActivatableWidget> DefaultClassPtr = TSoftClassPtr<UAccelByteWarsActivatableWidget>(DefaultClassString);
	TutorialModuleData.DefaultUIClass = UAccelByteWarsActivatableWidget::StaticClass();

	TutorialModuleData.bIsActive = UAccelByteWarsDataAsset::GetMetadataForAsset<bool>(GenerateAssetIdFromCodeName(InCodeName), GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, bIsActive));
	TutorialModuleData.bIsStarterModeActive = UAccelByteWarsDataAsset::GetMetadataForAsset<bool>(GenerateAssetIdFromCodeName(InCodeName), GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, bIsStarterModeActive));

#pragma region "Online Session"
	TutorialModuleData.bOnlineSessionModule = UAccelByteWarsDataAsset::GetMetadataForAsset<bool>(GenerateAssetIdFromCodeName(InCodeName), GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, bOnlineSessionModule));

	FString SessionClassString = TutorialModuleData.bIsStarterModeActive ?
	   UAccelByteWarsDataAsset::GetMetadataForAsset<FString>(TutorialModuleAssetId, GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, StarterOnlineSessionClass)):
	   UAccelByteWarsDataAsset::GetMetadataForAsset<FString>(TutorialModuleAssetId, GET_MEMBER_NAME_CHECKED(UTutorialModuleDataAsset, DefaultOnlineSessionClass));
	if (TSoftClassPtr<UOnlineSession> SessionClassPtr = TSoftClassPtr<UOnlineSession>(SessionClassString))
	{
		TutorialModuleData.OnlineSessionClass = SessionClassPtr.Get();
	}
#pragma endregion 

	return TutorialModuleData;
}

FPrimaryAssetId UTutorialModuleDataAsset::GenerateAssetIdFromCodeName(const FString& InCodeName)
{
	return FPrimaryAssetId(UTutorialModuleDataAsset::TutorialModuleAssetType, FName(*InCodeName));
}

FString UTutorialModuleDataAsset::GetCodeNameFromAssetId(const FPrimaryAssetId& AssetId)
{
	check(AssetId.PrimaryAssetType == UTutorialModuleDataAsset::TutorialModuleAssetType);
	return AssetId.PrimaryAssetName.ToString();
}

TSubclassOf<UAccelByteWarsActivatableWidget> UTutorialModuleDataAsset::GetTutorialModuleUIClass()
{
	return IsStarterModeActive() ? StarterUIClass : DefaultUIClass;
}

TSubclassOf<UTutorialModuleSubsystem> UTutorialModuleDataAsset::GetTutorialModuleSubsystemClass()
{
	return IsStarterModeActive() ? StarterSubsystemClass : DefaultSubsystemClass;
}

bool UTutorialModuleDataAsset::IsActiveAndDependenciesChecked() const
{
	bool bIsDependencySatisfied = true;
	for (const UTutorialModuleDataAsset* Dependency : TutorialModuleDependencies)
	{
		if (!Dependency) continue;

		if (!Dependency->IsActiveAndDependenciesChecked())
		{
			bIsDependencySatisfied = false;
			break;
		}
	}
	
	return !bIsDependencySatisfied ? false : bIsActive;
}

void UTutorialModuleDataAsset::OverridesIsActive(const bool bInIsActive)
{
	bOverriden = true;
	bIsActive = bInIsActive;
}

void UTutorialModuleDataAsset::ResetOverrides()
{
	bOverriden = false;
}

#pragma region "Online Session"
TSubclassOf<UTutorialModuleOnlineSession> UTutorialModuleDataAsset::GetTutorialModuleOnlineSessionClass()
{
	return IsStarterModeActive() ? StarterOnlineSessionClass : DefaultOnlineSessionClass;
}
#pragma endregion

void UTutorialModuleDataAsset::ValidateDataAssetProperties()
{
	// Validate Default's class properties.
	ValidateClassProperty(DefaultUIClass, LastDefaultUIClass, false);
	ValidateClassProperty(DefaultSubsystemClass, LastDefaultSubsystemClass, false);

	// Validate Starter's class properties.
	ValidateClassProperty(StarterUIClass, LastStarterUIClass, true);
	ValidateClassProperty(StarterSubsystemClass, LastStarterSubsystemClass, true);

	// Validate Default's and Starter class properties for OnlineSession module
	if (bOnlineSessionModule)
	{
		ValidateClassProperty(DefaultOnlineSessionClass, LastDefaultOnlineSessionClass, false);
		ValidateClassProperty(StarterOnlineSessionClass, LastStarterOnlineSessionClass, true);
	}

	ValidateGeneratedWidgets();
	ValidateFTUEDialogues();
}

bool UTutorialModuleDataAsset::ValidateClassProperty(TSubclassOf<UAccelByteWarsActivatableWidget>& UIClass, TSubclassOf<UAccelByteWarsActivatableWidget>& LastUIClass, const bool IsStarterClass)
{
	// Check if the class is used by other Tutorial Module or not.
	if (UIClass.Get() && UIClass.GetDefaultObject()->AssociateTutorialModule != nullptr
		&& UIClass.GetDefaultObject()->AssociateTutorialModule != this)
	{
#if UE_EDITOR
		ShowPopupMessage(
			FString::Printf(TEXT("UI Class %s is already being used by %s Tutorial Module"),
				*UIClass.Get()->GetName(),
				*UIClass.GetDefaultObject()->AssociateTutorialModule->GetName()));
#endif
		UIClass = nullptr;
	}

	// Reset the last class first to makes sure the references are clear.
	if (LastUIClass.Get() && LastUIClass.GetDefaultObject()
		&& LastUIClass.GetDefaultObject()->AssociateTutorialModule == this)
	{
		LastUIClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	// Update the new class to points to this Tutorial Module.
	if (UIClass.Get() && UIClass.GetDefaultObject())
	{
		UIClass.GetDefaultObject()->AssociateTutorialModule =
			((IsStarterClass && IsStarterModeActive()) || (!IsStarterClass && !IsStarterModeActive())) ? this : nullptr;
	}

	// Cache the class reference.
	LastUIClass = UIClass;

	return UIClass != nullptr;
}

bool UTutorialModuleDataAsset::ValidateClassProperty(TSubclassOf<UTutorialModuleSubsystem>& SubsystemClass, TSubclassOf<UTutorialModuleSubsystem>& LastSubsystemClass, const bool IsStarterClass)
{
	// Check if the class is used by other Tutorial Module or not.
	if (SubsystemClass.Get() && SubsystemClass.GetDefaultObject()->AssociateTutorialModule != nullptr
		&& SubsystemClass.GetDefaultObject()->AssociateTutorialModule != this)
	{
#if UE_EDITOR
		ShowPopupMessage(
			FString::Printf(TEXT("Subsystem Class %s is already being used by %s Tutorial Module"),
				*SubsystemClass.Get()->GetName(),
				*SubsystemClass.GetDefaultObject()->AssociateTutorialModule->GetName()));
#endif
		SubsystemClass = nullptr;
	}

	// Reset the last class first to makes sure the references are clear.
	if (LastSubsystemClass.Get() && LastSubsystemClass.GetDefaultObject()
		&& LastSubsystemClass.GetDefaultObject()->AssociateTutorialModule == this)
	{
		LastSubsystemClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	// Update the new class to points to this Tutorial Module
	if (SubsystemClass.Get() && SubsystemClass.GetDefaultObject())
	{
		SubsystemClass.GetDefaultObject()->AssociateTutorialModule = ((IsStarterClass && IsStarterModeActive()) || (!IsStarterClass && !IsStarterModeActive())) ? this : nullptr;
	}

	// Cache the class reference.
	LastSubsystemClass = SubsystemClass;

	return SubsystemClass != nullptr;
}

bool UTutorialModuleDataAsset::ValidateClassProperty(TSubclassOf<UTutorialModuleOnlineSession>& OnlineSessionClass, TSubclassOf<UTutorialModuleOnlineSession>&LastOnlineSessionClass, const bool IsStarterClass)
{
	// Check if the class is used by other Tutorial Module or not.
	if (OnlineSessionClass.Get() && OnlineSessionClass.GetDefaultObject()->AssociateTutorialModule != nullptr
	   && OnlineSessionClass.GetDefaultObject()->AssociateTutorialModule != this)
	{
#if UE_EDITOR
		ShowPopupMessage(
		   FString::Printf(TEXT("Subsystem Class %s is already being used by %s Tutorial Module"),
			  *OnlineSessionClass.Get()->GetName(),
			  *OnlineSessionClass.GetDefaultObject()->AssociateTutorialModule->GetName()));
#endif
		OnlineSessionClass = nullptr;
	}

	// Reset the last class first to makes sure the references are clear.
	if (LastOnlineSessionClass.Get() && LastOnlineSessionClass.GetDefaultObject()
	   && LastOnlineSessionClass.GetDefaultObject()->AssociateTutorialModule == this)
	{
		LastOnlineSessionClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	// Update the new class to points to this Tutorial Module
	if (OnlineSessionClass.Get() && OnlineSessionClass.GetDefaultObject())
	{
		OnlineSessionClass.GetDefaultObject()->AssociateTutorialModule =
		   ((IsStarterClass && IsStarterModeActive()) || (!IsStarterClass && !IsStarterModeActive())) ? this : nullptr;
	}

	// Cache the class reference.
	LastOnlineSessionClass = OnlineSessionClass;

	return OnlineSessionClass != nullptr;
}

void UTutorialModuleDataAsset::CleanUpDataAssetProperties()
{
	TutorialModuleDependencies.Empty();
	TutorialModuleDependents.Empty();

	for (FTutorialModuleGeneratedWidget& GeneratedWidget : GeneratedWidgets)
	{
		for (TSubclassOf<UAccelByteWarsActivatableWidget>& TargetWidgetClass : GeneratedWidget.TargetWidgetClasses)
		{
			if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
			{
				continue;
			}

			TargetWidgetClass.GetDefaultObject()->GeneratedWidgets.RemoveAll([this](const FTutorialModuleGeneratedWidget* Temp)
			{
				return Temp->OwnerTutorialModule == this;
			});
		}
	}
	GeneratedWidgets.Empty();

	if (DefaultUIClass.Get())
	{
		DefaultUIClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	if (DefaultSubsystemClass.Get())
	{
		DefaultSubsystemClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	if (StarterUIClass.Get())
	{
		StarterUIClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}

	if (StarterSubsystemClass.Get())
	{
		StarterSubsystemClass.GetDefaultObject()->AssociateTutorialModule = nullptr;
	}
}

#pragma region "Generated Widgets"
void UTutorialModuleDataAsset::ValidateGeneratedWidgets()
{
	// Clean up last generated widgets metadata to avoid duplication.
	for (FTutorialModuleGeneratedWidget& LastGeneratedWidget : LastGeneratedWidgets)
	{
		UTutorialModuleDataAsset::GeneratedWidgetUsedIds.Remove(LastGeneratedWidget.WidgetId);

		LastGeneratedWidget.DefaultTutorialModuleWidgetClass = nullptr;
		LastGeneratedWidget.StarterTutorialModuleWidgetClass = nullptr;

		LastGeneratedWidget.OtherTutorialModule = nullptr;

		for (TSubclassOf<UAccelByteWarsActivatableWidget>& TargetWidgetClass : LastGeneratedWidget.TargetWidgetClasses)
		{
			if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
			{
				continue;
			}

			TargetWidgetClass.GetDefaultObject()->GeneratedWidgets.RemoveAll([this](const FTutorialModuleGeneratedWidget* Temp)
			{
				return !Temp || !Temp->OwnerTutorialModule || Temp->OwnerTutorialModule == this;
			});
		}
	}
	for (FTutorialModuleGeneratedWidget& GeneratedWidget : GeneratedWidgets)
	{
		for (TSubclassOf<UAccelByteWarsActivatableWidget>& TargetWidgetClass : GeneratedWidget.TargetWidgetClasses)
		{
			if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
			{
				continue;
			}

			TargetWidgetClass.GetDefaultObject()->GeneratedWidgets.RemoveAll([this](const FTutorialModuleGeneratedWidget* Temp)
			{
				return !Temp || !Temp->OwnerTutorialModule || Temp->OwnerTutorialModule == this;
			});
		}
	}

	// Assign fresh generated widget to the target widget class.
	for (FTutorialModuleGeneratedWidget& GeneratedWidget : GeneratedWidgets)
	{
		// Clean up unnecessary references.
		if (GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::GENERIC_WIDGET_ENTRY_BUTTON &&
			GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::GENERIC_WIDGET)
		{
			GeneratedWidget.GenericWidgetClass = nullptr;
		}
		if (GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::OTHER_TUTORIAL_MODULE_ENTRY_BUTTON &&
			GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::OTHER_TUTORIAL_MODULE_WIDGET)
		{
			GeneratedWidget.OtherTutorialModule = nullptr;
		}
		if ((GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::TUTORIAL_MODULE_ENTRY_BUTTON &&
			GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::TUTORIAL_MODULE_WIDGET &&
			GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::OTHER_TUTORIAL_MODULE_ENTRY_BUTTON &&
			GeneratedWidget.WidgetType != ETutorialModuleGeneratedWidgetType::OTHER_TUTORIAL_MODULE_WIDGET) ||
			GeneratedWidget.TutorialModuleWidgetClassType != ETutorialModuleWidgetClassType::ASSOCIATE_WIDGET_CLASS)
		{
			GeneratedWidget.DefaultTutorialModuleWidgetClass = nullptr;
			GeneratedWidget.StarterTutorialModuleWidgetClass = nullptr;
		}

		// Assign the owner of the generated widget metadata to this Tutorial Module.
		GeneratedWidget.OwnerTutorialModule = this;

		// Check if the widget id is already used.
		if (UTutorialModuleDataAsset::GeneratedWidgetUsedIds.Contains(GeneratedWidget.WidgetId))
		{
#if UE_EDITOR
			ShowPopupMessage(FString::Printf(TEXT("%s widget id is already used. Widget id must be unique."), *GeneratedWidget.WidgetId));
#endif
			GeneratedWidget.WidgetId = TEXT("");
		}
		else if (!GeneratedWidget.WidgetId.IsEmpty())
		{
			UTutorialModuleDataAsset::GeneratedWidgetUsedIds.Add(GeneratedWidget.WidgetId);
		}

		// Assign the generated widget to the target widget class.
		if (IsActiveAndDependenciesChecked())
		{
			for (TSubclassOf<UAccelByteWarsActivatableWidget>& TargetWidgetClass : GeneratedWidget.TargetWidgetClasses)
			{
				if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
				{
					continue;
				}
				TargetWidgetClass.GetDefaultObject()->GeneratedWidgets.Add(&GeneratedWidget);
			}
		}
	}

	LastGeneratedWidgets = GeneratedWidgets;
}
#pragma endregion

#pragma region "First Time User Experience (FTUE)"
void UTutorialModuleDataAsset::ValidateFTUEDialogues()
{
	// Clean up last FTUE dialogues metadata to avoid duplication.
	for (auto& LastFTUEDialogue : LastFTUEDialogues)
	{
		UTutorialModuleDataAsset::FTUEDialogueUsedIds.Remove(LastFTUEDialogue.FTUEId);

		for (auto& TargetWidgetClass : LastFTUEDialogue.TargetWidgetClasses)
		{
			if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
			{
				continue;
			}

			TargetWidgetClass.GetDefaultObject()->FTUEDialogues.RemoveAll([this](const FFTUEDialogueModel Temp)
			{
				return !Temp.OwnerTutorialModule || Temp.OwnerTutorialModule == this;
			});
		}
	}
	for (auto& FTUEDialogue : FTUEDialogues)
	{
		for (auto& TargetWidgetClass : FTUEDialogue.TargetWidgetClasses)
		{
			if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
			{
				continue;
			}

			TargetWidgetClass.GetDefaultObject()->FTUEDialogues.RemoveAll([this](const FFTUEDialogueModel Temp)
			{
				return !Temp.OwnerTutorialModule || Temp.OwnerTutorialModule == this;
			});
		}
	}

	// Refresh FTUE dialogues metadata.
	for (auto& FTUEDialogue : FTUEDialogues)
	{
		FTUEDialogue.OwnerTutorialModule = this;
		FTUEDialogue.bIsAlreadyShown = false;

		// Reset button metadata if not used.
		switch (FTUEDialogue.ButtonType)
		{
		case FFTUEDialogueButtonType::NO_BUTTON:
			FTUEDialogue.Button1.Reset();
		case FFTUEDialogueButtonType::ONE_BUTTON:
			FTUEDialogue.Button2.Reset();
			break;
		}

		// Reset highlighted widget metadata if not used.
		if (!FTUEDialogue.bHighlightWidget)
		{
			FTUEDialogue.TargetWidgetClassToHighlight = nullptr;
			FTUEDialogue.TargetWidgetNameToHighlight = FString("");
		}

		// Check if the FTUE id is already used.
		if (UTutorialModuleDataAsset::FTUEDialogueUsedIds.Contains(FTUEDialogue.FTUEId))
		{
#if UE_EDITOR
			ShowPopupMessage(FString::Printf(TEXT("%s FTUE id is already used. FTUE id must be unique."), *FTUEDialogue.FTUEId));
#endif
			FTUEDialogue.FTUEId = TEXT("");
		}
		else if (!FTUEDialogue.FTUEId.IsEmpty())
		{
			UTutorialModuleDataAsset::FTUEDialogueUsedIds.Add(FTUEDialogue.FTUEId);
		}

		// Assign FTUE dialogue metadata to the target widget class.
		if (IsActiveAndDependenciesChecked() && HasFTUE())
		{
			for (auto& TargetWidgetClass : FTUEDialogue.TargetWidgetClasses)
			{
				if (!TargetWidgetClass || !TargetWidgetClass.GetDefaultObject())
				{
					continue;
				}
				TargetWidgetClass.GetDefaultObject()->FTUEDialogues.Add(FTUEDialogue);
			}
		}
	}

	LastFTUEDialogues = FTUEDialogues;
}
#pragma endregion

void UTutorialModuleDataAsset::PostLoad()
{
	Super::PostLoad();

	ValidateDataAssetProperties();
}

#if WITH_EDITOR
void UTutorialModuleDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ValidateDataAssetProperties();
}

void UTutorialModuleDataAsset::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	CodeName = TEXT("");
	ValidateDataAssetProperties();
}

void UTutorialModuleDataAsset::FinishDestroy()
{
	CleanUpDataAssetProperties();

	Super::FinishDestroy();
}

void UTutorialModuleDataAsset::ShowPopupMessage(const FString& Message) const
{
	FNotificationInfo Info(FText::FromString(Message));
	Info.Image = FCoreStyle::Get().GetBrush("MessageLog.Error");
	Info.ExpireDuration = 10.0f;
	Info.FadeInDuration = 0.25f;
	Info.FadeOutDuration = 0.5f;

	FSlateNotificationManager::Get().AddNotification(Info);
}
#endif