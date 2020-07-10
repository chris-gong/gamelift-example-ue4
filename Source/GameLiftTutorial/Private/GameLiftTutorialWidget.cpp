// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "GameLiftTutorialPlayerState.h"
#include "GameLiftTutorialGameState.h"
#include "GameLiftTutorialGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UGameLiftTutorialWidget::NativeConstruct() {
	Super::NativeConstruct();

	TeamNameTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeamName"));
	TeammateCountTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeammateCount"));
	EventTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_Event"));
	PingTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_Ping"));

	GetWorld()->GetTimerManager().SetTimer(SetTeammateCountHandle, this, &UGameLiftTutorialWidget::SetTeammateCount, 1.0f, true, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(SetLatestEventHandle, this, &UGameLiftTutorialWidget::SetLatestEvent, 1.0f, true, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(SetAveragePlayerLatencyHandle, this, &UGameLiftTutorialWidget::SetAveragePlayerLatency, 1.0f, true, 1.0f);
}

void UGameLiftTutorialWidget::NativeDestruct() {
	GetWorld()->GetTimerManager().ClearTimer(SetTeammateCountHandle);
	GetWorld()->GetTimerManager().ClearTimer(SetLatestEventHandle);
	GetWorld()->GetTimerManager().ClearTimer(SetAveragePlayerLatencyHandle);
	Super::NativeDestruct();
}

void UGameLiftTutorialWidget::SetTeammateCount() {
	FString OwningPlayerTeam;
	APlayerState* OwningPlayerState = GetOwningPlayerState();

	if (OwningPlayerState != nullptr) {
		AGameLiftTutorialPlayerState* OwningGameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(OwningPlayerState);
		if (OwningGameLiftTutorialPlayerState != nullptr) {
			OwningPlayerTeam = OwningGameLiftTutorialPlayerState->Team;
			TeamNameTextBlock->SetText(FText::FromString("Team Name: " + OwningPlayerTeam));
		}
	}

	if (OwningPlayerTeam.Len() > 0) {
		TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

		int TeammateCount = 0;

		for (APlayerState* PlayerState : PlayerStates) {
			if (PlayerState != nullptr) {
				AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
				if (GameLiftTutorialPlayerState != nullptr && GameLiftTutorialPlayerState->Team.Equals(OwningPlayerTeam)) {
					TeammateCount++;
				}
			}
		}

		TeammateCountTextBlock->SetText(FText::FromString("Teammate Count: " + FString::FromInt(TeammateCount)));
	}
}

void UGameLiftTutorialWidget::SetLatestEvent() {
	FString LatestEvent;
	FString WinningTeam;
	AGameStateBase* GameState = GetWorld()->GetGameState();

	if (GameState != nullptr) {
		AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
		if (GameLiftTutorialGameState != nullptr) {
			LatestEvent = GameLiftTutorialGameState->LatestEvent;
			WinningTeam = GameLiftTutorialGameState->WinningTeam;
		}
	}

	if (LatestEvent.Len() > 0) {
		if (LatestEvent.Equals("GameEnded")) {
			FString OwningPlayerTeam;
			APlayerState* OwningPlayerState = GetOwningPlayerState();

			if (OwningPlayerState != nullptr) {
				AGameLiftTutorialPlayerState* OwningGameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(OwningPlayerState);
				if (OwningGameLiftTutorialPlayerState != nullptr) {
					OwningPlayerTeam = OwningGameLiftTutorialPlayerState->Team;
				}
			}

			if (WinningTeam.Len() > 0 && OwningPlayerTeam.Len() > 0) {
				FString GameOverMessage = "You and the " + OwningPlayerTeam;
				if (OwningPlayerTeam.Equals(WinningTeam)) {
					EventTextBlock->SetText(FText::FromString(GameOverMessage + " won!"));
				}
				else {
					EventTextBlock->SetText(FText::FromString(GameOverMessage + " lost :("));
				}
			}
		}
		else {
			EventTextBlock->SetText(FText::FromString(LatestEvent));
		}
	}
}

void UGameLiftTutorialWidget::SetAveragePlayerLatency() {
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr) {
		UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
		if (GameLiftTutorialGameInstance != nullptr) {
			float TotalPlayerLatency = 0.0f;
			for (float PlayerLatency : GameLiftTutorialGameInstance->PlayerLatencies) {
				TotalPlayerLatency += PlayerLatency;
			}

			float AveragePlayerLatency = 60.0f;

			if (TotalPlayerLatency > 0) {
				AveragePlayerLatency = TotalPlayerLatency / GameLiftTutorialGameInstance->PlayerLatencies.Num();

				FString PingString = "Ping: " + FString::FromInt(FMath::RoundToInt(AveragePlayerLatency)) + "ms";
				PingTextBlock->SetText(FText::FromString(PingString));
			}
		}
	}
}