// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "Core/UI/Components/Prompt/FTUE/FTUEModels.h"
#include "Core/AssetManager/TutorialModules/TutorialModuleDataAsset.h"

FFTUEDialogueModel* FFTUEDialogueModel::GetMetadataById(const FString& FTUEId)
{
    return *UTutorialModuleDataAsset::GetCachedFTUEDialogues().
        FindByPredicate([FTUEId](const FFTUEDialogueModel* Temp)
        {
            return Temp->FTUEId == FTUEId;
        }
    );

    return nullptr;
}
