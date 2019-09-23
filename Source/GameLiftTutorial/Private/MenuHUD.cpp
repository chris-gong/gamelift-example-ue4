// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuHUD.h"
#include "UserWidget.h"
#include "Kismet/GameplayStatics.h"

AMenuHUD::AMenuHUD() {
	static ConstructorHelpers::FClassFinder<UUserWidget> MenuObj(TEXT("/Game/UI/Widgets/UI_Menu"));
	MenuWidgetClass = MenuObj.Class;
}

void AMenuHUD::BeginPlay() {
	Super::BeginPlay();
	// allowing user to interact with ui
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr) {
		PlayerController->bShowMouseCursor = true;
	}

	// adding UMG widget
	if (MenuWidgetClass != nullptr)
	{
		UUserWidget* MenuWidget = CreateWidget<UUserWidget>(GetWorld(), MenuWidgetClass);

		if (MenuWidget != nullptr)
		{
			MenuWidget->AddToViewport();
		}
	}
}
