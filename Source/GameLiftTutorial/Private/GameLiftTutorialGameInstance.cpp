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

	HttpModule = &FHttpModule::Get();
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