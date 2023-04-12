// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/AssetManager/AccelByteWarsDataAsset.h"
#include "Core/Settings/GameModeDataAssets.h"
#include "GameModeDataAsset.generated.h"


/**
 * 
 */
UCLASS()
class ACCELBYTEWARS_API UGameModeDataAsset : public UAccelByteWarsDataAsset
{
	GENERATED_BODY()

public:
	UGameModeDataAsset()
	{
		AssetType = UGameModeDataAsset::GameModeAssetType;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		if (CodeName.IsEmpty())
		{
			return Super::GetPrimaryAssetId();
		}

		// Use Alias for Game Mode AssetId for easy lookup
		return UGameModeDataAsset::GenerateAssetIdFromCodeName(CodeName);
	}
	
public:
	static FGameModeData GetGameModeDataByCodeName(const FString& InCodeName);

	static FPrimaryAssetId GenerateAssetIdFromCodeName(const FString& InCodeName);
	static FString GetCodeNameFromAssetId(const FPrimaryAssetId& AssetId);
	static FText GetDisplayNameByCodeName(const FString& InCodeName);
	static FPrimaryAssetId GetGameModeTypeForCodeName(const FString& InCodeName);

public:
	// Game mode type: FFA, TDM, or etc
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGameModeType GameModeType = EGameModeType::FFA;

	// Alias to set for this mode (needs to be unique)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString CodeName;

	// String representation of GameModeType PrimaryAssetId
	// Stored as string so it is asset registry searchable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable)
	FString GameModeTypeString;
	
	// Network type
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGameModeNetworkType NetworkType;
	
	// Either team game or not; If FFA then should be false.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsTeamGame;

	// Default max team count
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxTeamNum;

	// Default maximum supported player
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxPlayers;

	// Default match time
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MatchTime;
	
	// Default score limit
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ScoreLimit;

	// How many missiles per player can be fired at once
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FiredMissilesLimit;

	// How many lives player will start with
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartingLives;

	// Base missile score for killing an enemy
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BaseScoreForKill;

	// Missile score increment based on time
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TimeScoreIncrement;

	// Missile score delta time
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TimeScoreDeltaTime;

	// Base missile score for skimming (hovering close to planets / ships)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SkimInitialScore;

	// Missile skim score delta time
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SkimScoreDeltaTime;

	// Missile skim score multiplier
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SkimScoreAdditionalMultiplier;
	
public:
	static const FPrimaryAssetType GameModeAssetType;
};
