// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"

UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();

	JoinGameButton = (UButton*)GetWidgetFromName(TEXT("Button_JoinGame"));
}
