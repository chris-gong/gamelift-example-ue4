// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameLiftTutorialWidget.generated.h"

/**
 * 
 */
class UTextBlock;

UCLASS()
class GAMELIFTTUTORIAL_API UGameLiftTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameLiftTutorialWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UTextBlock* TeamNameTextBlock;
	UTextBlock* TeammateCountTextBlock;
	UTextBlock* EventTextBlock;

	FTimerHandle TeammateCountHandle;
	FTimerHandle CheckGameEventsHandle;

	FDelegateHandle TravelFailureDelegateHandle;

	void GetTeammateCount();
	void CheckGameEvents();

	UFUNCTION()
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString);
};
