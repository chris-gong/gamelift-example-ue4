// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialGameState.h"

#include "UnrealNetwork.h"

void AGameLiftTutorialGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameLiftTutorialGameState, LatestEvent);
	DOREPLIFETIME(AGameLiftTutorialGameState, WinningTeam);
}