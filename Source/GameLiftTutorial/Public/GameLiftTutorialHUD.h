// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameLiftTutorialHUD.generated.h"

class UUserWidget;
/**
 * 
 */
UCLASS()
class GAMELIFTTUTORIAL_API AGameLiftTutorialHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	AGameLiftTutorialHUD();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
		TSubclassOf<UUserWidget> GameWidgetClass;
};
