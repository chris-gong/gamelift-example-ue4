// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "TextReaderComponent.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
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
	RedirectUri = "https://cwz3ysqb50.execute-api.us-east-1.amazonaws.com/GoogleSignInSuccess";
	AwsCredsUrl = "https://5s45no77o5.execute-api.us-east-1.amazonaws.com/GetAwsCredentials";
	TestAuthUrl = "https://4pxv1tot0d.execute-api.us-east-1.amazonaws.com/test";
	//UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	HttpModule = &FHttpModule::Get();
}

void UMenuWidget::NativeConstruct() {
	Super::NativeConstruct();
	WebBrowser = (UWebBrowser*)GetWidgetFromName(TEXT("WebBrowser_Login"));

	FScriptDelegate LoginDelegate;
	LoginDelegate.BindUFunction(this, "CheckIfLoginSuccessful");
	WebBrowser->OnUrlChanged.Add(LoginDelegate);
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
				TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
				RequestObj->SetStringField(ParameterName, ParameterValue);
				//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("code: ") + ParameterValue);
				
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

			TSharedRef<IHttpRequest> TestAuthRequest = HttpModule->CreateRequest();
			TestAuthRequest->OnProcessRequestComplete().BindUObject(this, &UMenuWidget::OnTestAuthResponseReceived);
			TestAuthRequest->SetURL(TestAuthUrl);
			TestAuthRequest->SetVerb("POST");
			TestAuthRequest->SetHeader("Authorization", AccessToken);
			TestAuthRequest->ProcessRequest();
		}
		else {
		
		}
	}
	else {
		
	}
}

void UMenuWidget::OnTestAuthResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("Response from Authorized Lambda API: ") + Response->GetContentAsString());
}
