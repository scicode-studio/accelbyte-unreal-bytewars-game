// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Settings/GameModeDataAssets.h"
#include "Core/Settings/GlobalSettingsDataAsset.h"
#include "Engine/GameInstance.h"
#include "AccelByteWarsGameInstance.generated.h"

class ULoadingWidget;
class UAccelByteWarsBaseUI;
class UAccelByteWarsActivatableWidget;
class UAccelByteWarsButtonBase;

#pragma region "Structs and data storing purpose UObject declaration"
USTRUCT(BlueprintType)
struct FGameplayPlayerData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FUniqueNetIdRepl UniqueNetId;

	/**
	 * @brief Used for local game, since LocalPlayer does not have UniqueNetId (UE 5.1.0)
	 */
	UPROPERTY(BlueprintReadWrite)
	int32 ControllerId = 0;

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	FString AvatarURL;

	UPROPERTY(BlueprintReadWrite)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	float Score = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 NumLivesLeft = 1;

	bool operator==(const FGameplayPlayerData& Other) const
	{
		return UniqueNetId.IsValid() ? UniqueNetId == Other.UniqueNetId : ControllerId == Other.ControllerId;
	}
};

USTRUCT(BlueprintType)
struct FGameplayTeamData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	TArray<FGameplayPlayerData> TeamMembers;

	float GetTeamScore() const
	{
		float TotalScore = 0.0f;
		for (const FGameplayPlayerData& Player : TeamMembers)
		{
			TotalScore += Player.Score;
		}
		return TotalScore;
	}

	int32 GetTeamLivesLeft() const
	{
		int32 TotalLives = 0;
		for (const FGameplayPlayerData& Player : TeamMembers)
		{
			TotalLives += Player.NumLivesLeft;
		}
		return TotalLives;
	}

	int32 GetTeamKillCount() const
	{
		int32 TotalKillCount = 0;
		for (const FGameplayPlayerData& Player : TeamMembers)
		{
			TotalKillCount += Player.KillCount;
		}
		return TotalKillCount;
	}

	bool operator==(const FGameplayTeamData& Other) const
	{
		return TeamId == Other.TeamId && TeamMembers == Other.TeamMembers;
	}
};
#pragma endregion 

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPlayerChanged, ULocalPlayer*, LocalPlayer);
DECLARE_MULTICAST_DELEGATE(FOnGameInstanceShutdown);
DECLARE_LOG_CATEGORY_CLASS(LogAccelByteWarsGameInstance, Log, All);

UCLASS()
class ACCELBYTEWARS_API UAccelByteWarsGameInstance : public UGameInstance
{
	GENERATED_BODY()

	virtual void Shutdown() override;
	
public:
	/**
	 * @brief Transferring data between data - purpose. Do not use this directly. Use the one in GameState instead.
	 */
	TArray<FGameplayTeamData> Teams;
	FGameModeData GameSetup;

	UPROPERTY(BlueprintAssignable)
	FOnLocalPlayerChanged OnLocalPlayerAdded;

	UPROPERTY(BlueprintAssignable)
	FOnLocalPlayerChanged OnLocalPlayerRemoved;

	/**
	 * @brief Called on GameInstance::Shutdown | Just before the actual game exit. Will also be called on exit PIE.
	 */
	FOnGameInstanceShutdown OnGameInstanceShutdownDelegate;
	
private:
	/** This is the primary player*/
	TWeakObjectPtr<ULocalPlayer> PrimaryPlayer;

public:
	virtual int32 AddLocalPlayer(ULocalPlayer* NewLocalPlayer, FPlatformUserId UserId) override;

	virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Sounds)
	float GetMusicVolume();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Sounds)
	void SetMusicVolume(float InVolume);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Sounds)
	float GetSFXVolume();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Sounds)
	void SetSFXVolume(float InVolume);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = GameSettings)
	void LoadGameSettings();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = GameSettings)
	void SaveGameSettings();

	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	UAccelByteWarsBaseUI* GetBaseUIWidget();

	UFUNCTION(BlueprintPure)
	TSubclassOf<UAccelByteWarsButtonBase> GetDefaultButtonClass() const { return DefaultButtonClass; }

	FGameModeData GetGameModeDataByCodeName(const FString CodeName) const;
	FGameModeData GetGameModeDataByThirdPartyCodeName(const FString CodeName) const;

	/**
	 * @brief Get team color specified in GlobalSettingsDataAsset
	 * @param TeamId Target TeamId
	 * @return Configured team color
	 */
	UFUNCTION(BlueprintCallable)
	FLinearColor GetTeamColor(uint8 TeamId) const;

protected:
	UPROPERTY()
	UAccelByteWarsBaseUI* BaseUIWidget;

	bool bHasAddToViewportCalled = false;

	UPROPERTY(EditDefaultsOnly, NoClear)
	TSubclassOf<UAccelByteWarsBaseUI> BaseUIMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAccelByteWarsButtonBase> DefaultButtonClass;

	UPROPERTY(EditAnywhere)
	UGlobalSettingsDataAsset* GlobalSettingsDataAsset;

	UPROPERTY(EditAnywhere)
	UDataTable* GameModeDataTable;
};
