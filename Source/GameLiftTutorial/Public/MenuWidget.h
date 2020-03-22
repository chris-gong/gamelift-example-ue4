// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UserWidget.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "MenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWebBrowser;

// This class does not need to be modified.
UCLASS(BlueprintType)
class UMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	FHttpModule* HttpModule;

	FString LoginUrl;

	// Lambda APIs
	FString RedirectUri;
	FString ApiUrl;
	FString AwsCredsUrl;
	FString RetrievePlayerDataUrl;
	FString LookForMatchUrl;
	FString CancelMatchLookupUrl;
	FString PollMatchmakingUrl;

	// Widgets
	UWebBrowser* WebBrowser;
	UButton* MatchmakingButton;
	UTextBlock* WinsTextBlock;
	UTextBlock* LossesTextBlock;
	UTextBlock* LookingForMatchTextBlock;

	bool SearchingForGame;

	// Player Info, make this a struct or class or something later
	FString Wins;
	FString Losses;

	// Stuff for polling DynamoDb about ticket updates
	FTimerHandle PollMatchmakingHandle;

	void PollMatchmaking();

	// Delegate Functions
	UFUNCTION()
	void CheckIfLoginSuccessful();

	UFUNCTION()
	void OnMatchmakingButtonClicked();

	// OnResponse Received Functions
	void OnAwsTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnRetrievePlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnInitiateMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnPollMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnSignOutResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetNewTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

};