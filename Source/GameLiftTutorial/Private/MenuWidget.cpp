// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "TextReaderComponent.h"
#include "GameLiftClientSDK/Public/GameLiftClientObject.h"
#include "GameLiftClientSDK/Public/GameLiftClientApi.h"
#include "Components/Button.h"

UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	AccessKey = TextReader->ReadFile("Credentials/AWS_AccessKey.txt");
	SecretKey = TextReader->ReadFile("Credentials/AWS_SecretKey.txt");
	QueueName = TextReader->ReadFile("Credentials/AWS_QueueName.txt");
	Region = TextReader->ReadFile("Credentials/AWS_Region.txt");
	Client = UGameLiftClientObject::CreateGameLiftObject(AccessKey, SecretKey, "us-east-2");
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();

	JoinGameButton = (UButton*)GetWidgetFromName(TEXT("Button_JoinGame"));
	JoinGameButton->OnClicked.AddDynamic(this, &UMenuWidget::JoinGame);
}

void UMenuWidget::JoinGame() {

}