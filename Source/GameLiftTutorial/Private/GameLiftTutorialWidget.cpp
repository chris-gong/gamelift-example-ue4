// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"
#include "GameLiftTutorialPlayerState.h"
#include "GameLiftTutorialGameState.h"
#include "GameLiftTutorialGameInstance.h"

UGameLiftTutorialWidget::UGameLiftTutorialWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void UGameLiftTutorialWidget::NativeConstruct() {
	Super::NativeConstruct();

	TeamNameTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeamName"));
	TeammateCountTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeammateCount"));
	EventTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_Event"));

	GetWorld()->GetTimerManager().SetTimer(TeammateCountHandle, this, &UGameLiftTutorialWidget::GetTeammateCount, 1.0f, true, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(CheckGameEventsHandle, this, &UGameLiftTutorialWidget::CheckGameEvents, 1.0f, true, 1.0f);
	//UE_LOG(LogTemp, Warning, TEXT("GameLiftTutorialWidget NativeConstruct"));

}

void UGameLiftTutorialWidget::NativeDestruct() {
	Super::NativeDestruct();
	GetWorld()->GetTimerManager().ClearTimer(TeammateCountHandle);
	GetWorld()->GetTimerManager().ClearTimer(CheckGameEventsHandle);
	UE_LOG(LogTemp, Warning, TEXT("native destruct in UGameLiftTutorialWidget"));
}

FReply UGameLiftTutorialWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) {
	FReply Reply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	FString KeyName = InKeyEvent.GetKey().ToString();
	if (KeyName.Compare("Escape") == 0) {
		// Clean up first
		NativeDestruct();
		UGameInstance* GameInstance = GetGameInstance();
		if (GameInstance != nullptr) {
			UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
			if (GameLiftTutorialGameInstance != nullptr) {
				GameLiftTutorialGameInstance->Shutdown();
			}
		}
		if (!WITH_EDITOR) {
			// Quit the game
			FGenericPlatformMisc::RequestExit(false);
		}
	}
	return Reply;
}

void UGameLiftTutorialWidget::GetTeammateCount() {
	FString OwningPlayerTeam;
	// get owning player's team
	APlayerState* OwningPlayerState = GetOwningPlayerState();

	if (OwningPlayerState != nullptr) {
		AGameLiftTutorialPlayerState* OwningGameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(OwningPlayerState);
		if (OwningGameLiftTutorialPlayerState != nullptr) {
			OwningPlayerTeam = OwningGameLiftTutorialPlayerState->Team;
			TeamNameTextBlock->SetText(FText::FromString(FString("Team Name: ") + OwningPlayerTeam));
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("Owning Player Team: %s"), *(OwningPlayerTeam));
	if (OwningPlayerTeam.Len() > 0) {
		// gather all the players 
		TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;
		//UE_LOG(LogTemp, Warning, TEXT("Number of players acquired: %s"), *(FString::FromInt(PlayerStates.Num())));

		int TeammateCount = 0;

		for (APlayerState* PlayerState : PlayerStates) {
			if (PlayerState != nullptr) {
				AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
				if (GameLiftTutorialPlayerState != nullptr && GameLiftTutorialPlayerState->Team == OwningPlayerTeam) {
					TeammateCount++;
				}
			}
		}
		TeammateCountTextBlock->SetText(FText::FromString(FString("Teammate Count: ") + FString::FromInt(TeammateCount)));
	}
}

void UGameLiftTutorialWidget::CheckGameEvents() {
	AGameStateBase* GameState = GetWorld()->GetGameState();
	FString LatestEvent;
	FString WinningTeam;
	if (GameState != nullptr) {
		AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
		if (GameLiftTutorialGameState != nullptr) {
			LatestEvent = GameLiftTutorialGameState->LatestEvent;
			WinningTeam = GameLiftTutorialGameState->WinningTeam;
			//UE_LOG(LogTemp, Warning, TEXT("Latest event widget: %s"), *LatestEvent);
		}
	}

	if (LatestEvent.Len() > 0) {
		if (LatestEvent.Compare("GameEnded") == 0) {
			FString OwningPlayerTeam;
			// get owning player's team
			APlayerState* OwningPlayerState = GetOwningPlayerState();

			if (OwningPlayerState != nullptr) {
				AGameLiftTutorialPlayerState* OwningGameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(OwningPlayerState);
				if (OwningGameLiftTutorialPlayerState != nullptr) {
					OwningPlayerTeam = OwningGameLiftTutorialPlayerState->Team;
				}
			}
			if (WinningTeam.Len() > 0 && OwningPlayerTeam.Len() > 0) {
				FString GameOverMessage = FString("You and the ") + OwningPlayerTeam;
				if (OwningPlayerTeam.Compare(WinningTeam) == 0) {
					EventTextBlock->SetText(FText::FromString(GameOverMessage + FString(" won!")));
				}
				else {
					EventTextBlock->SetText(FText::FromString(GameOverMessage + FString(" lost :(")));
				}
			}
			GetWorld()->GetTimerManager().ClearTimer(TeammateCountHandle);
			GetWorld()->GetTimerManager().ClearTimer(CheckGameEventsHandle);

		}
		else {
			EventTextBlock->SetText(FText::FromString(LatestEvent));
		}
	}
}