// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UserWidget.h"
#include "MenuWidget.generated.h"

class UButton;
class UGameLiftClientObject;

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
	UPROPERTY()
	UButton* JoinGameButton;

	UPROPERTY()
	UGameLiftClientObject* Client;

	UPROPERTY()
	FString AccessKey;

	UPROPERTY()
	FString SecretKey;

	UPROPERTY()
	FString QueueName;

	UPROPERTY()
	FString Region;

	UFUNCTION()
	void JoinGame();
};

