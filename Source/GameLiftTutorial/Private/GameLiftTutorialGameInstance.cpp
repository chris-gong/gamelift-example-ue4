// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "TextReaderComponent.h"

UGameLiftTutorialGameInstance::UGameLiftTutorialGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));

	ApiUrl = TextReader->ReadFile("SecretUrls/ApiUrl.txt");
	CancelMatchLookupUrl = ApiUrl + "/cancelmatchlookup";
	SignOutUrl = ApiUrl + "/invalidateawscredentials";
	GetNewTokenUrl = ApiUrl + "/refreshawscredentials";

	HttpModule = &FHttpModule::Get();

	/*
	TODOS
	1) in shutdown, call the signouturl api
	2) make a setter function for the aws tokens that will be called by the menu widget and thereby initiate the timer function
	3) make another timer function that is called every 55 minutes to refresh the token
	4) go back to the widget class to only load the web browser if there is no aws token in the game instance
	*/

}

void UGameLiftTutorialGameInstance::Shutdown() {
	Super::Shutdown();
	UE_LOG(LogTemp, Warning, TEXT("game instance shutdown"));

	if (MatchmakingTicketId.Len() > 0) {
		UE_LOG(LogTemp, Warning, TEXT("Cancel matchmaking"));
		// cancel matchmaking request
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("ticketId", MatchmakingTicketId);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
			// send a get request to google discovery document to retrieve endpoints
			TSharedRef<IHttpRequest> CancelMatchLookupRequest = HttpModule->CreateRequest();
			CancelMatchLookupRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftTutorialGameInstance::OnEndMatchmakingResponseReceived);
			CancelMatchLookupRequest->SetURL(CancelMatchLookupUrl);
			CancelMatchLookupRequest->SetVerb("POST");
			CancelMatchLookupRequest->SetHeader("Content-Type", "application/json");
			CancelMatchLookupRequest->SetHeader("Authorization", AccessToken);
			CancelMatchLookupRequest->SetContentAsString(RequestBody);
			CancelMatchLookupRequest->ProcessRequest();
		}
	}
}

void UGameLiftTutorialGameInstance::OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {

}