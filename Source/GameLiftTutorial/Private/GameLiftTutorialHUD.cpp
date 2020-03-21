// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialHUD.h"
#include "UserWidget.h"
#include "GameLiftTutorialWidget.h"

AGameLiftTutorialHUD::AGameLiftTutorialHUD() {
	static ConstructorHelpers::FClassFinder<UUserWidget> TeamObj(TEXT("/Game/UI/Widgets/UI_Team"));
	TeamWidgetClass = TeamObj.Class;
}

void AGameLiftTutorialHUD::BeginPlay() {
	Super::BeginPlay();
	// turn off the mouse cursor, for this UI is not meant to be interactable with the client
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr) {
		PlayerController->bShowMouseCursor = false;
		//PlayerController->SetInputMode(FInputModeGameOnly());
	}

	// adding UMG widget
	if (TeamWidgetClass != nullptr)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), TeamWidgetClass);

		TeamWidget = Cast<UGameLiftTutorialWidget>(Widget);

		if (TeamWidget != nullptr)
		{
			TeamWidget->AddToViewport();
		}
	}
}

// TODO: Consider whether or not this should be done in the widget instead of in the character class
void AGameLiftTutorialHUD::SetTeamName(FString TeamName) {
	if (TeamWidget != nullptr) {
		TeamWidget->SetTeamNameText(TeamName);
	}
}