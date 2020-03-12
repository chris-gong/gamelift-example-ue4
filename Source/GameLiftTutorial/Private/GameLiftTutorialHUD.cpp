// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialHUD.h"
#include "UserWidget.h"

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
		UUserWidget* TeamWidget = CreateWidget<UUserWidget>(GetWorld(), TeamWidgetClass);

		if (TeamWidget != nullptr)
		{
			TeamWidget->AddToViewport();
		}
	}
}

