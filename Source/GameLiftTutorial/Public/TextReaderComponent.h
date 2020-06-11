// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TextReaderComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMELIFTTUTORIAL_API UTextReaderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTextReaderComponent();

	UFUNCTION()
		FString ReadFile(FString FilePath);
};
