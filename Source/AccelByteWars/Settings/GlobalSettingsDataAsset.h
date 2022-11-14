// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlobalSettingsDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FGlobalTeamSetup {

    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor itemColor;
};

/**
 * 
 */
UCLASS(BlueprintType)
class ACCELBYTEWARS_API UGlobalSettingsDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGlobalTeamSetup> GlobalTeamSetup;
};
