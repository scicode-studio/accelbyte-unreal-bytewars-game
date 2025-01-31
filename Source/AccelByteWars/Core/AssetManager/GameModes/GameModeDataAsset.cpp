// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.


#include "Core/AssetManager/GameModes/GameModeDataAsset.h"

const FPrimaryAssetType	UGameModeDataAsset::GameModeAssetType = TEXT("GameMode");

FGameModeData UGameModeDataAsset::GetGameModeDataByCodeName(const FString& InCodeName)
{
	const FPrimaryAssetId GameModeAssetId = GenerateAssetIdFromCodeName(InCodeName);

	FGameModeData GameModeData;

	GameModeData.GameModeType = static_cast<EGameModeType>(UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, GameModeType)));
	GameModeData.CodeName = InCodeName;
	GameModeData.DisplayName = UAccelByteWarsDataAsset::GetDisplayNameForAsset(GameModeAssetId);

	GameModeData.NetworkType = static_cast<EGameModeNetworkType>(UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, NetworkType)));
	GameModeData.bIsTeamGame = UAccelByteWarsDataAsset::GetMetadataForAsset<bool>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, bIsTeamGame));

	GameModeData.MaxTeamNum = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, MaxTeamNum));
	GameModeData.MaxPlayers = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, MaxPlayers));

	GameModeData.MatchTime = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, MatchTime));
	GameModeData.StartGameCountdown = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, StartGameCountdown));
	GameModeData.GameEndsShutdownCountdown = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, GameEndsShutdownCountdown));
	GameModeData.MinimumTeamCountToPreventAutoShutdown = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, MinimumTeamCountToPreventAutoShutdown));
	GameModeData.NotEnoughPlayerShutdownCountdown = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, NotEnoughPlayerCountdown));

	GameModeData.ScoreLimit = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, ScoreLimit));
	GameModeData.FiredMissilesLimit = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, FiredMissilesLimit));
	GameModeData.StartingLives = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, StartingLives));
	GameModeData.BaseScoreForKill = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, BaseScoreForKill));
	GameModeData.TimeScoreIncrement = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, TimeScoreIncrement));

	GameModeData.TimeScoreDeltaTime = UAccelByteWarsDataAsset::GetMetadataForAsset<float>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, TimeScoreDeltaTime));

	GameModeData.SkimInitialScore = UAccelByteWarsDataAsset::GetMetadataForAsset<int32>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, SkimInitialScore));

	GameModeData.SkimScoreDeltaTime = UAccelByteWarsDataAsset::GetMetadataForAsset<float>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, SkimScoreDeltaTime));
	GameModeData.SkimScoreAdditionalMultiplier = UAccelByteWarsDataAsset::GetMetadataForAsset<float>(GameModeAssetId, GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, SkimScoreAdditionalMultiplier));

	return GameModeData;
}

FPrimaryAssetId UGameModeDataAsset::GenerateAssetIdFromCodeName(const FString& InCodeName)
{
	return FPrimaryAssetId(UGameModeDataAsset::GameModeAssetType, FName(*InCodeName));
}

FString UGameModeDataAsset::GetCodeNameFromAssetId(const FPrimaryAssetId& AssetId)
{
	check(AssetId.PrimaryAssetType == UGameModeDataAsset::GameModeAssetType);
	return AssetId.PrimaryAssetName.ToString();
}

FText UGameModeDataAsset::GetDisplayNameByCodeName(const FString& InCodeName)
{
	return UAccelByteWarsDataAsset::GetDisplayNameForAsset(GenerateAssetIdFromCodeName(InCodeName));
}

FPrimaryAssetId UGameModeDataAsset::GetGameModeTypeForCodeName(const FString& InCodeName)
{
	const FString GameModeType = UAccelByteWarsDataAsset::GetMetadataForAsset<FString>(GenerateAssetIdFromCodeName(InCodeName), GET_MEMBER_NAME_CHECKED(UGameModeDataAsset, GameModeTypeString));
	return FPrimaryAssetId(GameModeType);
}