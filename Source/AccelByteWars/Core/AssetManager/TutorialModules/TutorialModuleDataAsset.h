// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Core/AssetManager/AccelByteWarsDataAsset.h"
#include "Core/AssetManager/AccelByteWarsAssetModels.h"
#include "Core/AssetManager/TutorialModules/TutorialModuleUtility.h"
#include "TutorialModuleDataAsset.generated.h"

class UTutorialModuleOnlineSession;
class UAccelByteWarsActivatableWidget;
class UTutorialModuleSubsystem;

UCLASS()
class ACCELBYTEWARS_API UTutorialModuleDataAsset : public UAccelByteWarsDataAsset
{
	GENERATED_BODY()

public:
	UTutorialModuleDataAsset()
	{
		AssetType = UTutorialModuleDataAsset::TutorialModuleAssetType;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		if (CodeName.IsEmpty())
		{
			return Super::GetPrimaryAssetId();
		}

		// Use Alias for Game Mode AssetId for easy lookup
		return UTutorialModuleDataAsset::GenerateAssetIdFromCodeName(CodeName);
	}

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void FinishDestroy();
#endif

	static FTutorialModuleData GetTutorialModuleDataByCodeName(const FString& InCodeName);
	static FPrimaryAssetId GenerateAssetIdFromCodeName(const FString& InCodeName);
	static FString GetCodeNameFromAssetId(const FPrimaryAssetId& AssetId);

	UFUNCTION(BlueprintPure)
	TSubclassOf<UAccelByteWarsActivatableWidget> GetTutorialModuleUIClass();
	
	UFUNCTION(BlueprintPure)
	TSubclassOf<UTutorialModuleSubsystem> GetTutorialModuleSubsystemClass();
	
	bool IsActiveAndDependenciesChecked() const;
	bool IsStarterModeActive() const { return bIsStarterModeActive; }

	void OverridesIsActive(const bool bInIsActive);
	void ResetOverrides();

	// Alias to set for this mode (needs to be unique)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Module")
	FString CodeName;

	static const FPrimaryAssetType TutorialModuleAssetType;

#pragma region "Online Session"
	UFUNCTION(BlueprintPure)
	bool GetIsOnlineSessionActivatable() const { return bOnlineSessionModule; }

	UFUNCTION(BlueprintPure)
	TSubclassOf<UTutorialModuleOnlineSession> GetTutorialModuleOnlineSessionClass();
#pragma endregion 

private:
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Tutorial Module")
	TSubclassOf<UAccelByteWarsActivatableWidget> DefaultUIClass;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Tutorial Module")
	TSubclassOf<UTutorialModuleSubsystem> DefaultSubsystemClass;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Tutorial Module Starter")
	TSubclassOf<UAccelByteWarsActivatableWidget> StarterUIClass;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Tutorial Module Starter")
	TSubclassOf<UTutorialModuleSubsystem> StarterSubsystemClass;

	UPROPERTY()
	bool bOverriden = false;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, meta = (EditCondition = "!bOverriden", HideEditConditionToggle), Category = "Tutorial Module")
	bool bIsActive = true;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Tutorial Module Starter", meta = (EditCondition = "bIsActive"))
	bool bIsStarterModeActive = false;

#pragma region "Online Session"
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Online Session Module")
	bool bOnlineSessionModule = false;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Online Session Module", meta = (EditCondition = "bOnlineSessionModule", EditConditionHides = "true"))
	TSubclassOf<UTutorialModuleOnlineSession> DefaultOnlineSessionClass;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Online Session Module", meta = (EditCondition = "bOnlineSessionModule", EditConditionHides = "true"))
	TSubclassOf<UTutorialModuleOnlineSession> StarterOnlineSessionClass;
#pragma endregion

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Module Dependencies", meta = (Tooltip = "Other Tutorial Modules that is required by this Tutorial Module", DisplayThumbnail = false, ShowOnlyInnerProperties))
	TArray<UTutorialModuleDataAsset*> TutorialModuleDependencies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Module Dependencies", meta = (Tooltip = "If set, this module will only be activated if one or more modules in this list is also active. Main purpose is to hide UIs.", DisplayThumbnail = false, ShowOnlyInnerProperties))
	TArray<UTutorialModuleDataAsset*> TutorialModuleDependents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Module Widgets", meta = (Tooltip = "Widgets that will be generated if this Tutorial Module active", ShowOnlyInnerProperties))
	TArray<FTutorialModuleGeneratedWidget> GeneratedWidgets;

private:
	void ValidateDataAssetProperties();
	bool ValidateClassProperty(TSubclassOf<UAccelByteWarsActivatableWidget>& UIClass, TSubclassOf<UAccelByteWarsActivatableWidget>& LastUIClass, const bool IsStarterClass);
	bool ValidateClassProperty(TSubclassOf<UTutorialModuleSubsystem>& SubsystemClass, TSubclassOf<UTutorialModuleSubsystem>& LastSubsystemClass, const bool IsStarterClass);
#pragma region "Online Session"
	bool ValidateClassProperty(TSubclassOf<UTutorialModuleOnlineSession>& OnlineSessionClass, TSubclassOf<UTutorialModuleOnlineSession>&LastOnlineSessionClass, const bool IsStarterClass);
#pragma endregion 

	void CleanUpDataAssetProperties();

#if WITH_EDITOR
	void ShowPopupMessage(const FString& Message) const;
#endif

	TSubclassOf<UAccelByteWarsActivatableWidget> LastDefaultUIClass;
	TSubclassOf<UTutorialModuleSubsystem> LastDefaultSubsystemClass;

	TSubclassOf<UAccelByteWarsActivatableWidget> LastStarterUIClass;
	TSubclassOf<UTutorialModuleSubsystem> LastStarterSubsystemClass;

#pragma region "Online Session"
	TSubclassOf<UTutorialModuleOnlineSession> LastDefaultOnlineSessionClass;
	TSubclassOf<UTutorialModuleOnlineSession> LastStarterOnlineSessionClass;
#pragma endregion 

	TArray<FTutorialModuleGeneratedWidget> LastGeneratedWidgets;

	static TSet<FString> GeneratedWidgetUsedIds;
};
