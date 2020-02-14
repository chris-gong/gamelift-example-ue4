// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UserWidget.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "MenuWidget.generated.h"

class UButton;
class UEditableTextBox;
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

private:
	FHttpModule* HttpModule;

	FString RedirectUri;
	FString AwsCredsUrl;
	FString TestAuthUrl;

	UWebBrowser* WebBrowser;

	FString IdToken;
	FString AccessToken;
	FString RefreshToken;

	UFUNCTION()
	void CheckIfLoginSuccessful();

	void OnAwsTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnTestAuthResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};