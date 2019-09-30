// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "TextReaderComponent.h"
#include "GameLiftClientSDK/Public/GameLiftClientObject.h"
#include "GameLiftClientSDK/Public/GameLiftClientApi.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogMenu);
UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	AccessKey = TextReader->ReadFile("Credentials/AWS_AccessKey.txt");
	SecretKey = TextReader->ReadFile("Credentials/AWS_SecretKey.txt");
	QueueName = TextReader->ReadFile("Credentials/AWS_QueueName.txt");
	Region = TextReader->ReadFile("Credentials/AWS_Region.txt");

	Client = UGameLiftClientObject::CreateGameLiftObject(AccessKey, SecretKey, Region);

	DescribeGameSessionQueuesEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);
	SearchGameSessionsEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);
	CreatePlayerSessionEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);
	StartGameSessionPlacementEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);
	DescribeGameSessionPlacementEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);

	JoinedGameSuccessfully = false;
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();

	JoinGameButton = (UButton*)GetWidgetFromName(TEXT("Button_JoinGame"));
	JoinGameButton->OnClicked.AddDynamic(this, &UMenuWidget::JoinGame);
}

void UMenuWidget::JoinGame() {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("button pressed"));
	JoinGameButton->SetIsEnabled(false);
	JoinedGameSuccessfully = false;

	DescribeGameSessionQueues(QueueName);
	DescribeGameSessionQueuesEvent->Wait();

	if (JoinedGameSuccessfully) {
		return;
	}

	const FString& PlacementId = GenerateRandomId();
	StartGameSessionPlacement(QueueName, 4, PlacementId);
	StartGameSessionPlacementEvent->Wait();
	
	if (!JoinedGameSuccessfully) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString("No game sessions available currently"));
		JoinGameButton->SetIsEnabled(true);
	}
#endif
}

void UMenuWidget::DescribeGameSessionQueues(const FString& QueueNameInput) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("describegamesessionqueues"));
	UGameLiftDescribeGameSessionQueues* DescribeGameSessionQueuesRequest = Client->DescribeGameSessionQueues(QueueNameInput);
	DescribeGameSessionQueuesRequest->OnDescribeGameSessionQueuesSuccess.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionQueuesSuccess);
	DescribeGameSessionQueuesRequest->OnDescribeGameSessionQueuesFailed.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionQueuesFailed);
	DescribeGameSessionQueuesRequest->Activate();
#endif
}

void UMenuWidget::OnDescribeGameSessionQueuesSuccess(const TArray<FString>& FleetARNs) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("on describegamesessionqueues sucess"));
	for (int i = 0; i < FleetARNs.Num(); i++) {
		FString FleetArn = FleetARNs[i];
		TArray<FString> FleetArnParsedOnce;
		FleetArn.ParseIntoArray(FleetArnParsedOnce, TEXT("arn:aws:gamelift:"), true);
		TArray<FString> FleetArnParsedAgain;
		FleetArnParsedOnce[0].ParseIntoArray(FleetArnParsedAgain, TEXT("::fleet/"), true);

		const FString& FleetId = FleetArnParsedAgain[1];
		SearchGameSessions(FleetId);
		SearchGameSessionsEvent->Wait();
		
		if (JoinedGameSuccessfully) {
			break;
		}
	}
	DescribeGameSessionQueuesEvent->Trigger();
#endif
}

void UMenuWidget::OnDescribeGameSessionQueuesFailed(const FString& ErrorMessage) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("on describegamesessionqueues failed"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	DescribeGameSessionQueuesEvent->Trigger();
#endif
}

void UMenuWidget::SearchGameSessions(const FString& FleetId) {
#if WITH_GAMELIFTCLIENTSDK
	UGameLiftSearchGameSessions* SearchGameSessionsRequest = Client->SearchGameSessions(FleetId, "", "", ""); // TODO: Have to handle other parameters
	SearchGameSessionsRequest->OnSearchGameSessionsSuccess.AddDynamic(this, &UMenuWidget::OnSearchGameSessionsSuccess);
	SearchGameSessionsRequest->OnSearchGameSessionsFailed.AddDynamic(this, &UMenuWidget::OnSearchGameSessionsFailed);
	SearchGameSessionsRequest->Activate();
#endif
}

void UMenuWidget::OnSearchGameSessionsSuccess(const TArray<FString>& GameSessionIds) {
#if WITH_GAMELIFTCLIENTSDK
	for (int i = 0; i < GameSessionIds.Num(); i++) {
		const FString& GameSessionId = GameSessionIds[i];
		const FString& PlayerSessionId = GenerateRandomId();

		CreatePlayerSession(GameSessionId, PlayerSessionId);
		CreatePlayerSessionEvent->Wait();

		if (JoinedGameSuccessfully) {
			break;
		}
	}

	SearchGameSessionsEvent->Trigger();
#endif
}

void UMenuWidget::OnSearchGameSessionsFailed(const FString& ErrorMessage) {
#if WITH_GAMELIFTCLIENTSDK
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	SearchGameSessionsEvent->Trigger();
#endif
}

void UMenuWidget::CreatePlayerSession(const FString& GameSessionId, const FString& PlayerSessionId) {
#if WITH_GAMELIFTCLIENTSDK
	UGameLiftCreatePlayerSession* CreatePlayerSessionRequest = Client->CreatePlayerSession(GameSessionId, PlayerSessionId);
	CreatePlayerSessionRequest->OnCreatePlayerSessionSuccess.AddDynamic(this, &UMenuWidget::OnCreatePlayerSessionSuccess);
	CreatePlayerSessionRequest->OnCreatePlayerSessionFailed.AddDynamic(this, &UMenuWidget::OnCreatePlayerSessionFailed);
	CreatePlayerSessionRequest->Activate();
#endif
}

void UMenuWidget::OnCreatePlayerSessionSuccess(const FString& IPAddress, const FString& Port, const FString& PlayerSessionID, const FString& PlayerSessionStatus) {
#if WITH_GAMELIFTCLIENTSDK
	if (PlayerSessionStatus.Equals("RESERVED", ESearchCase::IgnoreCase)) {
		FString LevelName = IPAddress + FString(":") + Port;
		const FString& Options = FString("?") + FString("PlayerSessionId=") + PlayerSessionID;

		UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelName), false, Options);

		JoinedGameSuccessfully = true;
	}

	CreatePlayerSessionEvent->Trigger();
#endif
}

void UMenuWidget::OnCreatePlayerSessionFailed(const FString& ErrorMessage) {
#if WITH_GAMELIFTCLIENTSDK
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	CreatePlayerSessionEvent->Trigger();
#endif
}

void UMenuWidget::StartGameSessionPlacement(const FString& QueueNameInput, const int& MaxPlayerCount, const FString& PlacementId) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("startgamesessionplacement"));
	UGameLiftStartGameSessionPlacement* StartGameSessionPlacementRequest = Client->StartGameSessionPlacement(QueueNameInput, MaxPlayerCount, PlacementId);
	StartGameSessionPlacementRequest->OnStartGameSessionPlacementSuccess.AddDynamic(this, &UMenuWidget::OnStartGameSessionPlacementSuccess);
	StartGameSessionPlacementRequest->OnStartGameSessionPlacementFailed.AddDynamic(this, &UMenuWidget::OnStartGameSessionPlacementFailed);
	StartGameSessionPlacementRequest->Activate();
#endif
}

void UMenuWidget::OnStartGameSessionPlacementSuccess(const FString& GameSessionId, const FString& PlacementId, const FString& Status) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("on startgamesessionplacement success"));
	if (Status.Equals("PENDING", ESearchCase::IgnoreCase)) {
		for (int i = 0; i < 10; i++) {
			StartGameSessionPlacementEvent->Wait(500);
			DescribeGameSessionPlacement(PlacementId);
			DescribeGameSessionPlacementEvent->Wait();
			if (JoinedGameSuccessfully) {
				break;
			}
		}
	}
	else if (Status.Equals("FULFILLED", ESearchCase::IgnoreCase)) {
		const FString& PlayerSessionId = GenerateRandomId();
		CreatePlayerSession(GameSessionId, PlayerSessionId);
		CreatePlayerSessionEvent->Wait();
	}
	StartGameSessionPlacementEvent->Trigger();
#endif
}

void UMenuWidget::OnStartGameSessionPlacementFailed(const FString& ErrorMessage) {
#if WITH_GAMELIFTCLIENTSDK
	UE_LOG(LogMenu, Log, TEXT("on startgamesessionplacement failed"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	StartGameSessionPlacementEvent->Trigger();
#endif
}

void UMenuWidget::DescribeGameSessionPlacement(const FString& PlacementId) {
#if WITH_GAMELIFTCLIENTSDK
	UGameLiftDescribeGameSessionPlacement* DescribeGameSessionPlacementRequest = Client->DescribeGameSessionPlacement(PlacementId);
	DescribeGameSessionPlacementRequest->OnDescribeGameSessionPlacementSuccess.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionPlacementSuccess);
	DescribeGameSessionPlacementRequest->OnDescribeGameSessionPlacementFailed.AddDynamic(this, &UMenuWidget::OnDescribeGameSessionPlacementFailed);
	DescribeGameSessionPlacementRequest->Activate();
#endif
}

void UMenuWidget::OnDescribeGameSessionPlacementSuccess(const FString& GameSessionId, const FString& PlacementId, const FString& Status) {
#if WITH_GAMELIFTCLIENTSDK
	if (Status.Equals("FULFILLED", ESearchCase::IgnoreCase)) {
		const FString& PlayerSessionId = GenerateRandomId();
		CreatePlayerSession(GameSessionId, PlayerSessionId);
		CreatePlayerSessionEvent->Wait();
	}
	DescribeGameSessionPlacementEvent->Trigger();
#endif
}

void UMenuWidget::OnDescribeGameSessionPlacementFailed(const FString& ErrorMessage) {
#if WITH_GAMELIFTCLIENTSDK
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
	DescribeGameSessionPlacementEvent->Trigger();
#endif
}

FString UMenuWidget::GenerateRandomId() {
	int RandOne = FMath::RandRange(0, 200000);
	int RandTwo = FMath::RandRange(0, 200000);
	int RandThree = FMath::RandRange(0, 200000);

	FString Id = FString::FromInt(RandOne) + FString("-") + FString::FromInt(RandTwo) + FString("-") + FString::FromInt(RandThree);

	return Id;
}