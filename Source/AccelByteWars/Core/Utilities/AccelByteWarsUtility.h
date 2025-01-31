// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"

typedef TSharedPtr<const FSlateBrush> FCacheBrush;
DECLARE_DELEGATE_OneParam(FOnImageReceived, FCacheBrush);

class ACCELBYTEWARS_API AccelByteWarsUtility
{
public:
	static void GetImageFromURL(const FString& Url, const FString& ImageId, const FOnImageReceived& OnReceived);
	static FCacheBrush GetImageFromCache(const FString& ImageId);

	/** @brief Always return positive value for Dividend % Modulus. If Modulus is zero, returns -1 as to prevent divide by zero exception. */
	static int32 PositiveModulo(const int32 Dividend, const int32 Modulus);

	static FString GetGameVersion();

private:
	static const TMap<FString, EImageFormat> ImageFormatMap;
};