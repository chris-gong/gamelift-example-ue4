// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialPlayerState.h"
#include "Net/UnrealNetwork.h"

void AGameLiftTutorialPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameLiftTutorialPlayerState, Team);
}
