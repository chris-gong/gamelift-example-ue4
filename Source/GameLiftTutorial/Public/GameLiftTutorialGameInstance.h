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
	// AWS Stuff
	FString IdToken;
	FString AccessToken;
	FString RefreshToken;

	// GameLift Stuff
	FString MatchmakingTicketId;

private:
	FHttpModule* HttpModule;

	FString ApiUrl;
	FString CancelMatchLookupUrl;
	FString SignOutUrl;
	FString GetNewTokenUrl;

	void OnEndMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
