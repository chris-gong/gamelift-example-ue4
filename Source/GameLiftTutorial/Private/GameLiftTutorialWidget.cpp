// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"
#include "GameLiftTutorialPlayerState.h"

UGameLiftTutorialWidget::UGameLiftTutorialWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void UGameLiftTutorialWidget::NativeConstruct() {
	Super::NativeConstruct();

	TeamNameTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeamName"));
	TeammateCountTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_TeammateCount"));
	EventTextBlock = (UTextBlock*) GetWidgetFromName(TEXT("TextBlock_Event"));

	GetWorld()->GetTimerManager().SetTimer(TeammateCountHandle, this, &UGameLiftTutorialWidget::GetTeammateCount, 1.0f, true, 1.0f);
}

void UGameLiftTutorialWidget::GetTeammateCount() {
	FString OwningPlayerTeam;
	// get owning player's team
	APlayerState* OwningPlayerState = GetOwningPlayerState();

	if (OwningPlayerState != nullptr) {
		AGameLiftTutorialPlayerState* OwningGameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(OwningPlayerState);
		if (OwningGameLiftTutorialPlayerState != nullptr) {
			OwningPlayerTeam = OwningGameLiftTutorialPlayerState->Team;
		}
	}

	if (OwningPlayerTeam.Len() > 0) {
		// gather all the players 
		TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

		int TeammateCount = 0;

		for (APlayerState* PlayerState : PlayerStates) {
			if (PlayerState != nullptr) {
				AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
				if (GameLiftTutorialPlayerState != nullptr && GameLiftTutorialPlayerState->Team == OwningPlayerTeam) {
					TeammateCount++;
				}
			}
		}

		TeammateCountTextBlock->SetText(FText::FromString(FString("TeammateCount: ") + FString::FromInt(TeammateCount)));
	}
}

void UGameLiftTutorialWidget::SetTeamNameText(FString TeamName) {
	TeamNameTextBlock->SetText(FText::FromString(FString("Team Name: ") + TeamName));
}