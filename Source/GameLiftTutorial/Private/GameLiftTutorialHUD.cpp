// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialHUD.h"
#include "Blueprint/UserWidget.h"

AGameLiftTutorialHUD::AGameLiftTutorialHUD() {
	static ConstructorHelpers::FClassFinder<UUserWidget> GameObj(TEXT("/Game/UI/Widgets/UI_Game"));
	GameWidgetClass = GameObj.Class;
}

void AGameLiftTutorialHUD::BeginPlay() {
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr) {
		PlayerController->bShowMouseCursor = false;
	}

	if (GameWidgetClass != nullptr) {
		UUserWidget* GameWidget = CreateWidget<UUserWidget>(GetWorld(), GameWidgetClass);
		if (GameWidget != nullptr) {
			GameWidget->AddToViewport();
		}
	}
}