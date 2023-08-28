﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.


#include "BrowseMatchP2PWidget.h"

#include "CommonButtonBase.h"
#include "Components/ListView.h"
#include "Core/UI/Components/AccelByteWarsWidgetSwitcher.h"
#include "TutorialModules/MatchSessionEssentials/MatchSessionEssentialsModels.h"
#include "TutorialModules/OnlineSessionUtils/AccelByteWarsOnlineSessionBase.h"

void UBrowseMatchP2PWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Btn_Refresh->OnClicked().AddUObject(this, &ThisClass::FindSessions, true);

	W_Parent = GetFirstOccurenceOuter<UBrowseMatchWidget>();
	if (!ensure(W_Parent))
	{
		return;
	}

	W_Parent->GetJoiningWidgetComponent()->OnCancelClicked.AddUObject(this, &ThisClass::CancelJoining);

	UOnlineSession* BaseOnlineSession = GetWorld()->GetGameInstance()->GetOnlineSession();
	if (!ensure(BaseOnlineSession))
	{
		return;
	}
	OnlineSession = Cast<UAccelByteWarsOnlineSessionBase>(BaseOnlineSession);

	OnlineSession->GetOnLeaveSessionCompleteDelegates()->AddUObject(this, &ThisClass::OnLeaveSessionComplete);
	OnlineSession->GetOnFindSessionsCompleteDelegates()->AddUObject(this, &ThisClass::OnFindSessionComplete);
	OnlineSession->GetOnJoinSessionCompleteDelegates()->AddUObject(this, &ThisClass::OnJoinSessionComplete);
	OnlineSession->GetOnSessionServerUpdateReceivedDelegates()->AddUObject(this, &ThisClass::OnSessionServerUpdateReceived);

	FindSessions(false);
}

void UBrowseMatchP2PWidget::NativeDestruct()
{
	Super::NativeDestruct();

	Btn_Refresh->OnClicked().RemoveAll(this);

	W_Parent->GetJoiningWidgetComponent()->OnCancelClicked.RemoveAll(this);

	OnlineSession->GetOnLeaveSessionCompleteDelegates()->RemoveAll(this);
	OnlineSession->GetOnFindSessionsCompleteDelegates()->RemoveAll(this);
	OnlineSession->GetOnJoinSessionCompleteDelegates()->RemoveAll(this);
	OnlineSession->GetOnSessionServerUpdateReceivedDelegates()->RemoveAll(this);
}

void UBrowseMatchP2PWidget::CancelJoining() const
{
	W_Parent->SetLoadingMessage(TEXT_LEAVING_SESSION, false, false);
	W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::JOIN_LOADING);

	OnlineSession->LeaveSession(
		OnlineSession->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession));
}

void UBrowseMatchP2PWidget::FindSessions(const bool bForce) const
{
	W_Parent->SetLoadingMessage(TEXT_LOADING_DATA, true, false);
	W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::BROWSE_LOADING);
	Btn_Refresh->SetIsEnabled(false);
	
	OnlineSession->FindSessions(
		OnlineSession->GetLocalUserNumFromPlayerController(GetOwningPlayer()),
		SessionsNumToQuery,
		bForce);
}

void UBrowseMatchP2PWidget::JoinSession(const FOnlineSessionSearchResult& SessionSearchResult) const
{
	W_Parent->SetLoadingMessage(TEXT_JOINING_SESSION, false, false);
	W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::JOIN_LOADING);

	OnlineSession->JoinSession(
		OnlineSession->GetLocalUserNumFromPlayerController(GetOwningPlayer()),
		OnlineSession->GetPredefinedSessionNameFromType(EAccelByteV2SessionType::GameSession),
		SessionSearchResult);
}

void UBrowseMatchP2PWidget::OnLeaveSessionComplete(FName SessionName, bool bSucceeded) const
{
	if (bSucceeded)
	{
		W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::BROWSE_NOT_EMPTY);
	}
	else
	{
		W_Parent->SetErrorMessage(TEXT_FAILED_TO_LEAVE_SESSION, false);
		W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::JOIN_ERROR);
	}
}

void UBrowseMatchP2PWidget::OnFindSessionComplete(
	const TArray<FMatchSessionEssentialInfo> SessionEssentialsInfo,
	bool bSucceeded)
{
	Btn_Refresh->SetIsEnabled(true);

	if (bSucceeded)
	{
		if (SessionEssentialsInfo.IsEmpty())
		{
			W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::BROWSE_EMPTY);
		}
		else
		{
			TArray<UMatchSessionData*> MatchSessionDatas;
			for (const FMatchSessionEssentialInfo& SessionEssentialInfo : SessionEssentialsInfo)
			{
				UMatchSessionData* MatchSessionData = NewObject<UMatchSessionData>();
				MatchSessionData->SessionEssentialInfo = SessionEssentialInfo;
				MatchSessionData->OnJoinButtonClicked.BindUObject(this, &ThisClass::JoinSession);
				MatchSessionDatas.Add(MatchSessionData);
			}

			W_Parent->GetListViewWidgetComponent()->ClearListItems();
			W_Parent->GetListViewWidgetComponent()->SetListItems<UMatchSessionData*>(MatchSessionDatas);
			W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::BROWSE_NOT_EMPTY);
		}
	}
	else
	{
		W_Parent->SetErrorMessage(TEXT_FAILED_TO_RETRIEVE_DATA, true);
		W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::BROWSE_ERROR);
	}
}

void UBrowseMatchP2PWidget::OnJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type CompletionType) const
{
	bool bSucceeded;
	FText ErrorMessage;

	switch (CompletionType)
	{
	case EOnJoinSessionCompleteResult::Success:
		bSucceeded = true;
		ErrorMessage = FText();
		break;
	case EOnJoinSessionCompleteResult::SessionIsFull:
		bSucceeded = false;
		ErrorMessage = TEXT_FAILED_SESSION_FULL;
		break;
	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		bSucceeded = false;
		ErrorMessage = TEXT_FAILED_SESSION_NULL;
		break;
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
		bSucceeded = false;
		ErrorMessage = TEXT_FAILED_TO_JOIN_SESSION;
		break;
	case EOnJoinSessionCompleteResult::AlreadyInSession:
		bSucceeded = false;
		ErrorMessage = TEXT_FAILED_ALREADY_IN_SESSION;
		break;
	case EOnJoinSessionCompleteResult::UnknownError:
		bSucceeded = false;
		ErrorMessage = TEXT_FAILED_TO_JOIN_SESSION;
		break;
	default:
		bSucceeded = true;
		ErrorMessage = FText();
	}

	W_Parent->SetErrorMessage(ErrorMessage, false);
	W_Parent->SetLoadingMessage(TEXT_JOINING_SESSION, false, true);
	W_Parent->SwitchContent(bSucceeded ?
		UBrowseMatchWidget::EContentType::JOIN_LOADING :
		UBrowseMatchWidget::EContentType::JOIN_ERROR);
}

void UBrowseMatchP2PWidget::OnSessionServerUpdateReceived(
	const FName SessionName,
	const FOnlineError& Error,
	const bool bHasClientTravelTriggered) const
{
	if (Error.bSucceeded && !bHasClientTravelTriggered)
	{
		// waiting for further update
		W_Parent->SetLoadingMessage(TEXT_JOINING_SESSION, false, false);
		W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::JOIN_LOADING);
	}
	else if (!bHasClientTravelTriggered && !Error.bSucceeded)
	{
		W_Parent->SetErrorMessage(TEXT_FAILED_TO_JOIN_SESSION, false);
		W_Parent->SwitchContent(UBrowseMatchWidget::EContentType::JOIN_ERROR);
	}
}

#pragma region "UI related"
UWidget* UBrowseMatchP2PWidget::NativeGetDesiredFocusTarget() const
{
	return Btn_Refresh;
}
#pragma endregion 
