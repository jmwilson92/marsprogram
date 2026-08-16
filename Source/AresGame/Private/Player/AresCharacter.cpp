#include "Player/AresCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "Interaction/AresInteractionComponent.h"
#include "Player/AresInputConfig.h"

AAresCharacter::AAresCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Roughly a 1.8 m person.
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	Interaction = CreateDefaultSubobject<UAresInteractionComponent>(TEXT("Interaction"));

	// First person: the body yaws with the controller, and the camera owns pitch.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->MaxWalkSpeed = WalkSpeed;
		Movement->JumpZVelocity = 420.0f;
		Movement->AirControl = 0.2f;
		// Earth. L_Surface overrides this per site in M5 (Moon 1.62, Mars 3.72).
		Movement->GravityScale = 1.0f;
	}
}

void AAresCharacter::EnsureInputConfig()
{
	if (!InputConfig)
	{
		InputConfig = NewObject<UAresInputConfig>(this, TEXT("AresInputConfig"));
		InputConfig->BuildDefaults();
	}
}

void AAresCharacter::ApplyMappingContext()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !InputConfig || !InputConfig->MappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(InputConfig->MappingContext, 0);
		}
	}
}

void AAresCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay and SetupPlayerInputComponent can arrive in either order
	// depending on how the pawn was possessed, so both build the config and
	// both are safe to run twice.
	EnsureInputConfig();
	ApplyMappingContext();
}

void AAresCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputConfig();
	ApplyMappingContext();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input || !InputConfig)
	{
		// Enhanced Input is the only supported path; the plugin is enabled in
		// Ares.uproject. If this fires, the project default input component
		// class has been changed.
		UE_LOG(LogTemp, Error, TEXT("AAresCharacter: expected a UEnhancedInputComponent."));
		return;
	}

	Input->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AAresCharacter::Move);
	Input->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AAresCharacter::Look);

	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Started, this, &AAresCharacter::SprintStart);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Completed, this, &AAresCharacter::SprintStop);

	Input->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &AAresCharacter::Interact);
}

void AAresCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}

	// Move relative to where the player is looking, flattened to the ground
	// plane so looking up does not slow you down.
	const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix YawMatrix(YawOnly);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void AAresCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	// Negated here rather than by an input modifier so the convention is
	// visible at the point of use: mouse down should pitch the view down.
	AddControllerPitchInput(-Axis.Y);
}

void AAresCharacter::Interact()
{
	if (Interaction)
	{
		Interaction->TryInteract();
	}
}

void AAresCharacter::SprintStart()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = SprintSpeed;
	}
}

void AAresCharacter::SprintStop()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}
