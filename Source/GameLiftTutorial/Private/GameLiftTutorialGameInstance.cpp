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
	4) go back to the widget class to only load the web browser if there is no aws token in the game instance
	5) update to the newest version of chrome for the web browser widget
	*/

}

void UGameLiftTutorialGameInstance::Shutdown() {
	Super::Shutdown();
	//GetWorld()->GetTimerManager().ClearTimer(GetNewTokenHandle); // can't do this in a shut down
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

	if (AccessToken.Len() > 0) {
		//UE_LOG(LogTemp, Warning, TEXT("Refresh Token: %s"), *RefreshToken);

		TSharedRef<IHttpRequest> SignOutRequest = HttpModule->CreateRequest();
		SignOutRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftTutorialGameInstance::OnSignOutResponseReceived);
		SignOutRequest->SetURL(SignOutUrl);
		SignOutRequest->SetVerb("GET");
		SignOutRequest->SetHeader("Content-Type", "application/json");
		SignOutRequest->SetHeader("Authorization", AccessToken);
		SignOutRequest->ProcessRequest();
	}
}

void UGameLiftTutorialGameInstance::SetAwsTokens(FString FAccessToken, FString FIdToken, FString FRefreshToken) {
	GetWorld()->GetTimerManager().ClearTimer(GetNewTokenHandle);
	this->AccessToken = FAccessToken;
	this->IdToken = FIdToken;
	this->RefreshToken = FRefreshToken;
	GetWorld()->GetTimerManager().SetTimer(GetNewTokenHandle, this, &UGameLiftTutorialGameInstance::RetrieveNewAccessToken, 1.0f, false, 3300.0f);
}

void UGameLiftTutorialGameInstance::RetrieveNewAccessToken() {
	GetWorld()->GetTimerManager().ClearTimer(GetNewTokenHandle);

	// make api request for refreshing the aws token
	TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
	RequestObj->SetStringField("refreshToken", RefreshToken);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

	if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer) && AccessToken.Len() > 0) {
		// send a get request to google discovery document to retrieve endpoints
		TSharedRef<IHttpRequest> PollMatchmakingRequest = HttpModule->CreateRequest();
		PollMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftTutorialGameInstance::OnGetNewTokenResponseReceived);
		PollMatchmakingRequest->SetURL(GetNewTokenUrl);
		PollMatchmakingRequest->SetVerb("POST");
		PollMatchmakingRequest->SetHeader("Content-Type", "application/json");
		PollMatchmakingRequest->SetHeader("Authorization", AccessToken);
		PollMatchmakingRequest->SetContentAsString(RequestBody);
		PollMatchmakingRequest->ProcessRequest();
	}
	else {
		// need to try to refresh the token quickly if for any reason attempts to do so fail
		GetWorld()->GetTimerManager().SetTimer(GetNewTokenHandle, this, &UGameLiftTutorialGameInstance::RetrieveNewAccessToken, 1.0f, false, 60.0f);
	}
}

void UGameLiftTutorialGameInstance::OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	UE_LOG(LogTemp, Warning, TEXT("Response from cancel matchmaking %s"), *(Response->GetContentAsString()));

	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());

			//TSharedPtr<FJsonObject> PlayerData = JsonObject->GetObjectField("playerData");

		}
		else {

		}
	}
	else {

	}
}

void UGameLiftTutorialGameInstance::OnSignOutResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	UE_LOG(LogTemp, Warning, TEXT("Response from signing out %s"), *(Response->GetContentAsString())); 

	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());

			//TSharedPtr<FJsonObject> PlayerData = JsonObject->GetObjectField("playerData");

		}
		else {

		}
	}
	else {

	}
}

void UGameLiftTutorialGameInstance::OnGetNewTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	//UE_LOG(LogTemp, Warning, TEXT("Response from getting new token %s"), *(Response->GetContentAsString()));

	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			AccessToken = JsonObject->GetStringField("accessToken");
			UE_LOG(LogTemp, Warning, TEXT("New access token assigned"));
			//UE_LOG(LogTemp, Warning, TEXT("New access token %s"), *(AccessToken));
			GetWorld()->GetTimerManager().SetTimer(GetNewTokenHandle, this, &UGameLiftTutorialGameInstance::RetrieveNewAccessToken, 1.0f, false, 3300.0f);
		}
		else {
			// need to try to refresh the token quickly if for any reason attempts to do so fail
			GetWorld()->GetTimerManager().SetTimer(GetNewTokenHandle, this, &UGameLiftTutorialGameInstance::RetrieveNewAccessToken, 1.0f, false, 60.0f);
		}
	}
	else {
		// need to try to refresh the token quickly if for any reason attempts to do so fail
		GetWorld()->GetTimerManager().SetTimer(GetNewTokenHandle, this, &UGameLiftTutorialGameInstance::RetrieveNewAccessToken, 1.0f, false, 60.0f);
	}
}