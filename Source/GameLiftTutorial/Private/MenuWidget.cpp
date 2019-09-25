// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "TextReaderComponent.h"
#include "GameLiftClientSDK/Public/GameLiftClientObject.h"
#include "GameLiftClientSDK/Public/GameLiftClientApi.h"
#include "Components/Button.h"
#include "Engine/Engine.h"

UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	AccessKey = TextReader->ReadFile("Credentials/AWS_AccessKey.txt");
	SecretKey = TextReader->ReadFile("Credentials/AWS_SecretKey.txt");
	QueueName = TextReader->ReadFile("Credentials/AWS_QueueName.txt");
	Region = TextReader->ReadFile("Credentials/AWS_Region.txt");

	Client = UGameLiftClientObject::CreateGameLiftObject(AccessKey, SecretKey, "us-east-2");

	DescribeGameSessionQueuesEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();

	JoinGameButton = (UButton*)GetWidgetFromName(TEXT("Button_JoinGame"));
	JoinGameButton->OnClicked.AddDynamic(this, &UMenuWidget::JoinGame);
}

void UMenuWidget::JoinGame() {
	JoinGameButton->SetIsEnabled(false);
	//DisableMouseEvents();

	/*FailedToJoinGame = false;
	SucceededToJoinGame = false;

	DescribeGameSessionQueues(QueueName);
	DescribeGameSessionQueuesEvent->Wait();

	if (AttemptToJoinGameFinished) {
		if (SucceededToJoinGame) {
			// don't reenable anything since game was successfully joined
		}
		else if (FailedToJoinGame) {
			JoinGameButton->SetIsEnabled(true);
			EnableMouseEvents();
		}
		return;
	}

	const FString& PlacementId = GenerateRandomId();
	StartGameSessionPlacement(QueueName, 100, PlacementId);
	StartGameSessionPlacementEvent->Wait();

	AttemptToJoinGameFinished = true;
	if (SucceededToJoinGame) {
		// don't reenable anything since game was successfully joined
	}
	else if (FailedToJoinGame) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString("No game sessions available currently"));
		JoinGameButton->SetIsEnabled(true);
		EnableMouseEvents();
	}*/
}

void UMenuWidget::DescribeGameSessionQueues(const FString& QueueNameInput) {
	UGameLiftDescribeGameSessionQueues* DescribeGameSessionQueuesRequest = Client->DescribeGameSessionQueues(QueueNameInput);
	DescribeGameSessionQueuesRequest->OnDescribeGameSessionQueuesSuccess.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionQueuesSuccess);
	DescribeGameSessionQueuesRequest->OnDescribeGameSessionQueuesFailed.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionQueuesFailed);
	DescribeGameSessionQueuesRequest->Activate();
}

void UMenuWidget::OnDescribeGameSessionQueuesSuccess(const TArray<FString>& FleetARNs) {
	for (int i = 0; i < FleetARNs.Num(); i++) {
		FString FleetArn = FleetARNs[i];
		TArray<FString> FleetArnParsedOnce;
		FleetArn.ParseIntoArray(FleetArnParsedOnce, TEXT("arn:aws:gamelift:"), true);
		TArray<FString> FleetArnParsedAgain;
		FleetArnParsedOnce[0].ParseIntoArray(FleetArnParsedAgain, TEXT("::fleet/"), true);

		const FString& FleetId = FleetArnParsedAgain[1];
		SearchGameSessions(FleetId);
		/*SearchGameSessionsEvent->Wait();

		if (AttemptToJoinGameFinished) {
			break;
		}*/
	}
	DescribeGameSessionQueuesEvent->Trigger();
}

void UMenuWidget::OnDescribeGameSessionQueuesFailed(const FString& ErrorMessage) {
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	DescribeGameSessionQueuesEvent->Trigger();
}

void UMenuWidget::SearchGameSessions(const FString& FleetId) {
	UGameLiftSearchGameSessions* SearchGameSessionsRequest = Client->SearchGameSessions(FleetId, "", "", ""); // TODO: Have to handle other parameters
	SearchGameSessionsRequest->OnSearchGameSessionsSuccess.AddDynamic(this, &UMenuWidget::OnSearchGameSessionsSuccess);
	SearchGameSessionsRequest->OnSearchGameSessionsFailed.AddDynamic(this, &UMenuWidget::OnSearchGameSessionsFailed);
	SearchGameSessionsRequest->Activate();
}

void UMenuWidget::OnSearchGameSessionsSuccess(const TArray<FString>& GameSessionIds) {

}

void UMenuWidget::OnSearchGameSessionsFailed(const FString& ErrorMessage) {
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
}