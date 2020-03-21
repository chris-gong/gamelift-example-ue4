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

	void SetTeamNameText(FString TeamName);

protected:
	virtual void NativeConstruct() override;

private:
	UTextBlock* TeamNameTextBlock;
	UTextBlock* TeammateCountTextBlock;
	UTextBlock* EventTextBlock;

	FTimerHandle TeammateCountHandle;
	FTimerHandle CheckGameEventsHandle;

	void GetTeammateCount();
	void CheckGameEvents();
};
