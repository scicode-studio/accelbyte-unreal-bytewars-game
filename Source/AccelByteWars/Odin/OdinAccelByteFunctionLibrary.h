// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OdinAccelByteFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ACCELBYTEWARS_API UOdinAccelByteFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category="Odin", meta=(WorldContext=WorldContextObject))
	static FName GetCurrentSessionName(const UObject* WorldContextObject);
};
