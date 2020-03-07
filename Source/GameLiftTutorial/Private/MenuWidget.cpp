// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "TextReaderComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Math/UnrealMathUtility.h"
#include "WebBrowser.h"
#include "IWebBrowserCookieManager.h"
#include "WebBrowserModule.h"
#include "Misc/Base64.h"

UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	// TODO: These three url variables, put these into a file eventually and read from that file and gitifnore that file
	RedirectUri = "https://cwz3ysqb50.execute-api.us-east-1.amazonaws.com/GoogleSignInSuccess";
	ApiUrl = "https://yjqjoq12ti.execute-api.us-east-1.amazonaws.com/test";
	AwsCredsUrl = "https://5s45no77o5.execute-api.us-east-1.amazonaws.com/GetAwsCredentials";
	RetrievePlayerDataUrl = ApiUrl + "/retrieveplayerdata";
	LookForMatchUrl = ApiUrl + "/lookformatch";
	CancelMatchLookupUrl = ApiUrl + "/cancelmatchlookup";
	//UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	HttpModule = &FHttpModule::Get();
	SearchingForGame = false;
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();
	WebBrowser = (UWebBrowser*)GetWidgetFromName(TEXT("WebBrowser_Login"));

	FScriptDelegate LoginDelegate;
	LoginDelegate.BindUFunction(this, "CheckIfLoginSuccessful");
	WebBrowser->OnUrlChanged.Add(LoginDelegate);

	MatchmakingButton = (UButton*)GetWidgetFromName(TEXT("Button_Matchmaking"));

	FScriptDelegate MatchmakingDelegate;
	MatchmakingDelegate.BindUFunction(this, "OnMatchmakingButtonClicked");
	MatchmakingButton->OnClicked.Add(MatchmakingDelegate);

	WinsTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Wins"));
	LossesTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Losses"));
	LookingForMatchTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_LookingForMatch"));
}

void UMenuWidget::CheckIfLoginSuccessful() {
	FString BrowserUrl = WebBrowser->GetUrl();
	FString Url;
	FString QueryParameters;

	if (BrowserUrl.Split("?", &Url, &QueryParameters)) {
		if (Url.Equals(RedirectUri)) {
			FString ParameterName;
			FString ParameterValue;
			if (QueryParameters.Split("=", &ParameterName, &ParameterValue)) {
				// # is not part of the code and usually appened to the end of query parameter
				ParameterValue = ParameterValue.Replace(*FString("#"), *FString("")); 
				TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
				RequestObj->SetStringField(ParameterName, ParameterValue);
				//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("code: ") + ParameterValue);
				//UE_LOG(LogTemp, Warning, TEXT("url query parameters after signin: %s"), *QueryParameters);

				//UE_LOG(LogTemp, Warning, TEXT("code: %s"), *ParameterValue);
				FString RequestBody;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

				if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
					// send a get request to google discovery document to retrieve endpoints
					TSharedRef<IHttpRequest> TokenRequest = HttpModule->CreateRequest();
					TokenRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnAwsTokenResponseReceived);
					TokenRequest->SetURL(AwsCredsUrl);
					TokenRequest->SetVerb("POST");
					TokenRequest->SetHeader("Content-Type", "application/json");
					TokenRequest->SetContentAsString(RequestBody);
					TokenRequest->ProcessRequest();
				}
			}
		}
	}
}

void UMenuWidget::OnMatchmakingButtonClicked() {
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("Matchmaking Button Clicked"));
	MatchmakingButton->SetIsEnabled(false);
	if (SearchingForGame) {
		// cancel matchmaking request
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("ticketId", MatchmakingTicketId);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
			// send a get request to google discovery document to retrieve endpoints
			TSharedRef<IHttpRequest> CancelMatchLookupRequest = HttpModule->CreateRequest();
			CancelMatchLookupRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnEndMatchmakingResponseReceived);
			CancelMatchLookupRequest->SetURL(CancelMatchLookupUrl);
			CancelMatchLookupRequest->SetVerb("POST");
			CancelMatchLookupRequest->SetHeader("Content-Type", "application/json");
			CancelMatchLookupRequest->SetHeader("Authorization", AccessToken);
			CancelMatchLookupRequest->SetContentAsString(RequestBody);
			CancelMatchLookupRequest->ProcessRequest();
		}
		else {
			MatchmakingButton->SetIsEnabled(true);
		}
	}
	else {
		// initiate matchmaking request
		TSharedRef<IHttpRequest> InitiateMatchmakingRequest = HttpModule->CreateRequest();
		InitiateMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnInitiateMatchmakingResponseReceived);
		InitiateMatchmakingRequest->SetURL(LookForMatchUrl);
		InitiateMatchmakingRequest->SetVerb("GET");
		InitiateMatchmakingRequest->SetHeader("Authorization", AccessToken);
		InitiateMatchmakingRequest->ProcessRequest();
	}
}

void UMenuWidget::OnAwsTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;
		
		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		
		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());
			//UE_LOG(LogTemp, Warning, TEXT("response: %s"), *(Response->GetContentAsString()));
			IdToken = JsonObject->GetStringField("id_token");
			AccessToken = JsonObject->GetStringField("access_token");
			RefreshToken = JsonObject->GetStringField("refresh_token");
			//UE_LOG(LogTemp, Warning, TEXT("AccessToken: %s"), *(AccessToken));

			TSharedRef<IHttpRequest> RetrievePlayerDataRequest = HttpModule->CreateRequest();
			RetrievePlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnRetrievePlayerDataResponseReceived);
			RetrievePlayerDataRequest->SetURL(RetrievePlayerDataUrl);
			RetrievePlayerDataRequest->SetVerb("GET");
			RetrievePlayerDataRequest->SetHeader("Authorization", AccessToken);
			RetrievePlayerDataRequest->ProcessRequest();
		}
		else {
		
		}
	}
	else {
		
	}
}

void UMenuWidget::OnRetrievePlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if(bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TSharedPtr<FJsonObject> PlayerData = JsonObject->GetObjectField("playerData");
			TSharedPtr<FJsonObject> WinsObject = PlayerData->GetObjectField("Wins");
			TSharedPtr<FJsonObject> LossesObject = PlayerData->GetObjectField("Losses");

			Wins = WinsObject->GetStringField("N");
			Losses = LossesObject->GetStringField("N");
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("Wins: ") + Wins + FString(" Losses: ") + Losses);
			WebBrowser->SetVisibility(ESlateVisibility::Hidden);
			MatchmakingButton->SetVisibility(ESlateVisibility::Visible);
			WinsTextBlock->SetVisibility(ESlateVisibility::Visible);
			LossesTextBlock->SetVisibility(ESlateVisibility::Visible);

			WinsTextBlock->SetText(FText::FromString("Wins: " + Wins));
			LossesTextBlock->SetText(FText::FromString("Losses: " + Losses));
		}
		else {

		}
	}
	else {

	}
}


void UMenuWidget::OnInitiateMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());

			MatchmakingTicketId = JsonObject->GetStringField("ticketId");
		}
		else {

		}
	}
	else {

	}
	UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
	ButtonText->SetText(FText::FromString("Cancel"));
	LookingForMatchTextBlock->SetVisibility(ESlateVisibility::Visible);

	SearchingForGame = !SearchingForGame;

	MatchmakingButton->SetIsEnabled(true);
}


void UMenuWidget::OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
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
	UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
	ButtonText->SetText(FText::FromString("Join Game"));
	LookingForMatchTextBlock->SetVisibility(ESlateVisibility::Hidden);

	SearchingForGame = !SearchingForGame;

	MatchmakingButton->SetIsEnabled(true);
}