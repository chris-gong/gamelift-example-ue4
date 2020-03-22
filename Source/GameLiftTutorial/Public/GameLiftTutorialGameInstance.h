// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GameLiftTutorialGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMELIFTTUTORIAL_API UGameLiftTutorialGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UGameLiftTutorialGameInstance(const FObjectInitializer& ObjectInitializer);

	virtual void Shutdown() override;

	void SetAwsTokens(FString AccessToken, FString IdToken, FString RefreshToken);

	// AWS Stuff
	FString IdToken;
	FString AccessToken;
	FString RefreshToken;

	// GameLift Stuff
	FString MatchmakingTicketId;

	FTimerHandle GetNewTokenHandle;

private:
	FHttpModule* HttpModule;

	FString ApiUrl;
	FString CancelMatchLookupUrl;
	FString SignOutUrl;
	FString GetNewTokenUrl;

	void RetrieveNewAccessToken();

	void OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnSignOutResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetNewTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

};
