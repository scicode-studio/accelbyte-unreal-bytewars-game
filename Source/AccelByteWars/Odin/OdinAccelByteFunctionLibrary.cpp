// Fill out your copyright notice in the Description page of Project Settings.


#include "OdinAccelByteFunctionLibrary.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemAccelByte/Private/AsyncTasks/SessionV2/OnlineAsyncTaskAccelByteCreateGameSessionV2.h"


FName UOdinAccelByteFunctionLibrary::GetCurrentSessionName(const UObject* WorldContextObject)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub != nullptr)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession); // Use the appropriate session name
			if (CurrentSession)
			{
				return CurrentSession->SessionName;
			}
		}
	}
	return "";
}
