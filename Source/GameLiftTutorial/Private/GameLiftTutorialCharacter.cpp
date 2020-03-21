// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutorialCharacter.h"
#include "GameLiftTutorialPlayerState.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Engine.h"
#include "UnrealNetwork.h"
#include "GameFramework/HUD.h"
#include "GameLiftTutorialHUD.h"

//////////////////////////////////////////////////////////////////////////
// AGameLiftTutorialCharacter

AGameLiftTutorialCharacter::AGameLiftTutorialCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	TeamColorSet = false;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGameLiftTutorialCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGameLiftTutorialCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGameLiftTutorialCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGameLiftTutorialCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGameLiftTutorialCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AGameLiftTutorialCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AGameLiftTutorialCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGameLiftTutorialCharacter::OnResetVR);
}

void AGameLiftTutorialCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGameLiftTutorialCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AGameLiftTutorialCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AGameLiftTutorialCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGameLiftTutorialCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AGameLiftTutorialCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AGameLiftTutorialCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AGameLiftTutorialCharacter::OnRep_PlayerState() {
	Super::OnRep_PlayerState();
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("OnRep_PlayerState called"));

	if (!TeamColorSet) {
		APlayerState* AState = GetPlayerState();
		if (AState != nullptr) {
			AGameLiftTutorialPlayerState* State = Cast<AGameLiftTutorialPlayerState>(AState);
			if (State != nullptr) {
				FString Team = State->Team;
				if (Team.Len() > 0) {
					// set player color
					UMaterialInstanceDynamic* PlayerMaterial = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
					if (Team.Compare("cowboys") == 0) {
						PlayerMaterial->SetVectorParameterValue("BodyColor", FLinearColor::Red);
					}
					else if (Team.Compare("aliens") == 0) {
						PlayerMaterial->SetVectorParameterValue("BodyColor", FLinearColor::Blue);
					}

					GetMesh()->SetMaterial(0, PlayerMaterial);

					// set player text
					AController* FController = GetController();
					if (FController != nullptr) {
						APlayerController* PlayerController = Cast<APlayerController>(FController);
						if (PlayerController != nullptr) {
							AHUD* HUD = PlayerController->GetHUD();
							if (HUD != nullptr) {
								//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString("hud exists"));

								AGameLiftTutorialHUD* GameLiftTutorialHUD = Cast<AGameLiftTutorialHUD>(HUD);
								GameLiftTutorialHUD->SetTeamName(Team);
							}
						}
					}
					TeamColorSet = true;
				}
			}
		}
	}
}
