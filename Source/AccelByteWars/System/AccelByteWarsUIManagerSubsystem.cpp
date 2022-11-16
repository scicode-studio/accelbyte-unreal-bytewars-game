// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AccelByteWarsUIManagerSubsystem.h"
#include "UI/GameUIController.h"
#include "UI/AccelByteWarsBaseUI.h"
#include "GameFramework/HUD.h"
#include "Player/CommonLocalPlayer.h"

UAccelByteWarsUIManagerSubsystem::UAccelByteWarsUIManagerSubsystem()
{
	
}

void UAccelByteWarsUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAccelByteWarsUIManagerSubsystem::Tick), 0.0f);
}

void UAccelByteWarsUIManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

bool UAccelByteWarsUIManagerSubsystem::Tick(float DeltaTime)
{
	SyncRootLayoutVisibilityToShowHUD();
	
	return true;
}

void UAccelByteWarsUIManagerSubsystem::SyncRootLayoutVisibilityToShowHUD()
{
	if (const UGameUIController* Policy = GetCurrentUIController())
	{
		for (const ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
		{
			bool bShouldShowUI = true;
			
			if (const APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
			{
				const AHUD* HUD = PC->GetHUD();

				if (HUD && !HUD->bShowHUD)
				{
					bShouldShowUI = false;
				}
			}

			if (UAccelByteWarsBaseUI* RootLayout = Policy->GetRootLayout(CastChecked<UCommonLocalPlayer>(LocalPlayer)))
			{
				const ESlateVisibility DesiredVisibility = bShouldShowUI ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
				if (DesiredVisibility != RootLayout->GetVisibility())
				{
					RootLayout->SetVisibility(DesiredVisibility);	
				}
			}
		}
	}
}