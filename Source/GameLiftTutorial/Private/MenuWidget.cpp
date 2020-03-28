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
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "IWebBrowserSingleton.h"
#include "GameLiftTutorialGameInstance.h"

UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));

	LoginUrl = TextReader->ReadFile("SecretUrls/LoginUrl.txt");
	RedirectUri = TextReader->ReadFile("SecretUrls/RedirectUri.txt");
	ApiUrl = TextReader->ReadFile("SecretUrls/ApiUrl.txt");
	AwsCredsUrl = TextReader->ReadFile("SecretUrls/AwsCredsUrl.txt");
	RetrievePlayerDataUrl = ApiUrl + "/retrieveplayerdata";
	LookForMatchUrl = ApiUrl + "/lookformatch";
	CancelMatchLookupUrl = ApiUrl + "/cancelmatchlookup";
	PollMatchmakingUrl = ApiUrl + "/pollmatchmaking";

	HttpModule = &FHttpModule::Get();
	SearchingForGame = false;
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();
	WebBrowser = (UWebBrowser*)GetWidgetFromName(TEXT("WebBrowser_Login"));

	MatchmakingButton = (UButton*)GetWidgetFromName(TEXT("Button_Matchmaking"));

	FScriptDelegate MatchmakingDelegate;
	MatchmakingDelegate.BindUFunction(this, "OnMatchmakingButtonClicked");
	MatchmakingButton->OnClicked.Add(MatchmakingDelegate);

	WinsTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Wins"));
	LossesTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Losses"));
	MatchmakingEventTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_MatchmakingEvent"));

	//TODO: check whether or not aws tokens exist, otherwise, hide the webbrowser and make the other stuff visible
	FString AccessToken;
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr) {
		UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
		if (GameLiftTutorialGameInstance != nullptr) {
			AccessToken = GameLiftTutorialGameInstance->AccessToken;
		}
	}

	if (AccessToken.Len() > 0) {
		TSharedRef<IHttpRequest> RetrievePlayerDataRequest = HttpModule->CreateRequest();
		RetrievePlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnRetrievePlayerDataResponseReceived);
		RetrievePlayerDataRequest->SetURL(RetrievePlayerDataUrl);
		RetrievePlayerDataRequest->SetVerb("GET");
		RetrievePlayerDataRequest->SetHeader("Authorization", AccessToken);
		RetrievePlayerDataRequest->ProcessRequest();
	}
	else {
		// clear the webcache folder in the saved folder
		IWebBrowserSingleton* WebBrowserSingleton = IWebBrowserModule::Get().GetSingleton();
		if (WebBrowserSingleton)
		{
			TOptional<FString> DefaultContext;
			TSharedPtr<IWebBrowserCookieManager> CookieManager = WebBrowserSingleton->GetCookieManager(DefaultContext);
			if (CookieManager.IsValid())
				CookieManager->DeleteCookies();
		}

		WebBrowser->LoadURL(LoginUrl);

		FScriptDelegate LoginDelegate;
		LoginDelegate.BindUFunction(this, "CheckIfLoginSuccessful");
		WebBrowser->OnUrlChanged.Add(LoginDelegate);

		WebBrowser->SetVisibility(ESlateVisibility::Visible);
	}
}

//TODO: Should this be moved to some ondestroy event in the gameinstance class
void UMenuWidget::NativeDestruct() {
	Super::NativeDestruct();
	GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
	UE_LOG(LogTemp, Warning, TEXT("native destruct in umenuwdiget"));
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

	FString MatchmakingTicketId;
	FString AccessToken;
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr) {
		UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
		if (GameLiftTutorialGameInstance != nullptr) {
			MatchmakingTicketId = GameLiftTutorialGameInstance->MatchmakingTicketId;
			AccessToken = GameLiftTutorialGameInstance->AccessToken;
			//UE_LOG(LogTemp, Warning, TEXT("accesstoken =  %s"), *(AccessToken));
		}
	}

	if (SearchingForGame) {
		GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle); // stop searching for a match
		UE_LOG(LogTemp, Warning, TEXT("Cancel matchmaking"));
		
		if (MatchmakingTicketId.Len() > 0) {
			// cancel matchmaking request
			TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
			RequestObj->SetStringField("ticketId", MatchmakingTicketId);

			FString RequestBody;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

			if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer) && AccessToken.Len() > 0) {
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
				UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
				ButtonText->SetText(FText::FromString("Join Game"));
				MatchmakingEventTextBlock->SetText(FText::FromString(""));

				SearchingForGame = !SearchingForGame;

				MatchmakingButton->SetIsEnabled(true);
			}
		}
		else {
			UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
			ButtonText->SetText(FText::FromString("Join Game"));
			MatchmakingEventTextBlock->SetText(FText::FromString(""));

			SearchingForGame = !SearchingForGame;

			MatchmakingButton->SetIsEnabled(true);
		}
	}
	else {
		if (AccessToken.Len() > 0) {
			UE_LOG(LogTemp, Warning, TEXT("initiate matchmaking"));
			// initiate matchmaking request
			TSharedRef<IHttpRequest> InitiateMatchmakingRequest = HttpModule->CreateRequest();
			InitiateMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnInitiateMatchmakingResponseReceived);
			InitiateMatchmakingRequest->SetURL(LookForMatchUrl);
			InitiateMatchmakingRequest->SetVerb("GET");
			InitiateMatchmakingRequest->SetHeader("Authorization", AccessToken);
			InitiateMatchmakingRequest->ProcessRequest();
		}
	}
}

void UMenuWidget::PollMatchmaking() {
	GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Polling Matchmaking");

	FString MatchmakingTicketId;
	FString AccessToken;
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr) {
		UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
		if (GameLiftTutorialGameInstance != nullptr) {
			MatchmakingTicketId = GameLiftTutorialGameInstance->MatchmakingTicketId;
			AccessToken = GameLiftTutorialGameInstance->AccessToken;
		}
	}

	if (MatchmakingTicketId.Len() > 0) {
		// poll for matchmaking status
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("ticketId", MatchmakingTicketId);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer) && AccessToken.Len() > 0) {
			// send a get request to google discovery document to retrieve endpoints
			TSharedRef<IHttpRequest> PollMatchmakingRequest = HttpModule->CreateRequest();
			PollMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnPollMatchmakingResponseReceived);
			PollMatchmakingRequest->SetURL(PollMatchmakingUrl);
			PollMatchmakingRequest->SetVerb("POST");
			PollMatchmakingRequest->SetHeader("Content-Type", "application/json");
			PollMatchmakingRequest->SetHeader("Authorization", AccessToken);
			PollMatchmakingRequest->SetContentAsString(RequestBody);
			PollMatchmakingRequest->ProcessRequest();
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);
		}
	}
	else {
		GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);
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
 
			UGameInstance* GameInstance = GetGameInstance();
			if (GameInstance != nullptr) {
				UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
				if (GameLiftTutorialGameInstance != nullptr) {
					FString IdToken = JsonObject->GetStringField("id_token");
					FString AccessToken = JsonObject->GetStringField("access_token");
					FString RefreshToken = JsonObject->GetStringField("refresh_token");

					GameLiftTutorialGameInstance->SetAwsTokens(AccessToken, IdToken, RefreshToken);

					TSharedRef<IHttpRequest> RetrievePlayerDataRequest = HttpModule->CreateRequest();
					RetrievePlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnRetrievePlayerDataResponseReceived);
					RetrievePlayerDataRequest->SetURL(RetrievePlayerDataUrl);
					RetrievePlayerDataRequest->SetVerb("GET");
					RetrievePlayerDataRequest->SetHeader("Authorization", AccessToken);
					RetrievePlayerDataRequest->ProcessRequest();
				}
			}
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
			MatchmakingEventTextBlock->SetVisibility(ESlateVisibility::Visible);

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
	//UE_LOG(LogTemp, Warning, TEXT("Response from initiate matchmaking %s"), *(Response->GetContentAsString()));

	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());

			FString MatchmakingTicketId = JsonObject->GetStringField("ticketId");

			UGameInstance* GameInstance = GetGameInstance();
			if (GameInstance != nullptr) {
				UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
				if (GameLiftTutorialGameInstance != nullptr) {
					//UE_LOG(LogTemp, Warning, TEXT("Assigning matchmaking ticket %s"), *(MatchmakingTicketId));
					GameLiftTutorialGameInstance->MatchmakingTicketId = MatchmakingTicketId;
				}
			}

			GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);

			UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
			ButtonText->SetText(FText::FromString("Cancel"));
			MatchmakingEventTextBlock->SetText(FText::FromString("Currently looking for a match"));

			SearchingForGame = !SearchingForGame;
		}
	}
	MatchmakingButton->SetIsEnabled(true);
}


void UMenuWidget::OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	//UE_LOG(LogTemp, Warning, TEXT("Response from cancel matchmaking %s"), *(Response->GetContentAsString()));

	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());

			bool CancellationSuccessful = JsonObject->GetBoolField("success");
			if (CancellationSuccessful) {
				UGameInstance* GameInstance = GetGameInstance();
				if (GameInstance != nullptr) {
					UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
					if (GameLiftTutorialGameInstance != nullptr) {
						GameLiftTutorialGameInstance->MatchmakingTicketId = FString("");
					}
					UE_LOG(LogTemp, Warning, TEXT("Cancellation successful"));
				}
			}
		}
		else {

		}
	}
	else {

	}
	
	UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
	ButtonText->SetText(FText::FromString("Join Game"));
	MatchmakingEventTextBlock->SetText(FText::FromString(""));

	SearchingForGame = !SearchingForGame;

	MatchmakingButton->SetIsEnabled(true);
}

void UMenuWidget::OnPollMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("response: ") + Response->GetContentAsString());
	//UE_LOG(LogTemp, Warning, TEXT("Response from poll matchmaking: %s"), *(Response->GetContentAsString()));
	if (bWasSuccessful) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TSharedPtr<FJsonObject> Ticket = JsonObject->GetObjectField("ticket");
			FString TicketStatus = Ticket->GetObjectField("Type")->GetStringField("S");

			if (TicketStatus.Compare("MatchmakingSearching") == 0) {
				// continue to poll matchmaking
				GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);
			}
			else if (TicketStatus.Compare("MatchmakingSucceeded") == 0) {
				UGameInstance* GameInstance = GetGameInstance();
				if (GameInstance != nullptr) {
					UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
					if (GameLiftTutorialGameInstance != nullptr) {
						GameLiftTutorialGameInstance->MatchmakingTicketId = FString("");
					}
				}
				// get the game session and player session details and connect to the server
				TSharedPtr<FJsonObject> GameSessionInfo = Ticket->GetObjectField("GameSessionInfo")->GetObjectField("M");
				FString IpAddress = GameSessionInfo->GetObjectField("IpAddress")->GetStringField("S");
				FString Port = GameSessionInfo->GetObjectField("Port")->GetStringField("N");
				FString PlayerSessionId = Ticket->GetObjectField("PlayerSessionId")->GetStringField("S");
				FString PlayerId = Ticket->GetObjectField("PlayerId")->GetStringField("S");
				FString LevelName = IpAddress + FString(":") + Port;
				const FString& Options = FString("?") + FString("PlayerSessionId=") + PlayerSessionId + FString("?PlayerId=") + PlayerId;

				UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelName), false, Options);
			}
			else if (TicketStatus.Compare("MatchmakingTimedOut") == 0 || TicketStatus.Compare("MatchmakingCancelled") == 0 || TicketStatus.Compare("MatchmakingFailed") == 0) {
				// stop calling the PollMatchmaking function
				UGameInstance* GameInstance = GetGameInstance();
				if (GameInstance != nullptr) {
					UGameLiftTutorialGameInstance* GameLiftTutorialGameInstance = Cast<UGameLiftTutorialGameInstance>(GameInstance);
					if (GameLiftTutorialGameInstance != nullptr) {
						GameLiftTutorialGameInstance->MatchmakingTicketId = FString("");
					}
				}
				// modify the text
				UTextBlock* ButtonText = (UTextBlock*)MatchmakingButton->GetChildAt(0);
				ButtonText->SetText(FText::FromString("Join Game"));
				if (TicketStatus.Compare("MatchmakingCancelled") == 0) {
					MatchmakingEventTextBlock->SetText(FText::FromString("Matchmaking request was cancelled. Please try again"));
				}
				else if (TicketStatus.Compare("MatchmakingTimedOut") == 0) {
					MatchmakingEventTextBlock->SetText(FText::FromString("Matchmaking request timed out. Please try again"));
				}
				else if(TicketStatus.Compare("MatchmakingFailed") == 0) {
					MatchmakingEventTextBlock->SetText(FText::FromString("Matchmaking request failed. Please try again"));
				}

				SearchingForGame = !SearchingForGame;
			}
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);
		}
	}
	else {
		GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &UMenuWidget::PollMatchmaking, 1.0f, false, 10.0f);
	}
}

void UMenuWidget::OnSignOutResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {

}

void UMenuWidget::OnGetNewTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {

}