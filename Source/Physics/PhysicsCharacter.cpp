// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsCharacter.h"
#include "PhysicsProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Components/StaticMeshComponent.h>
#include <PhysicsEngine/PhysicsHandleComponent.h>

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APhysicsCharacter::APhysicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	m_PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	m_bIsGrabbing = false;
	m_fCurrentGrabDistance = 0.f;
	m_bIsSprinting = false;
}

void APhysicsCharacter::BeginPlay()
{
	Super::BeginPlay();

	m_fCurrentStamina = m_MaxStamina;
}

void APhysicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// @TODO: Stamina update
  bool bIsMoving = !GetVelocity().IsNearlyZero(1.) && GetCharacterMovement()->IsMovingOnGround();
	if (m_bIsSprinting && bIsMoving)
	{
    m_fCurrentStamina = FMath::Clamp(m_fCurrentStamina - m_StaminaDepletionRate * DeltaSeconds, 0.f, m_MaxStamina);
    if (m_fCurrentStamina <= 0.f)
    {
      SetIsSprinting(false);
      m_bSprintLocked = true;
    }
	}
	else
	{
		m_fCurrentStamina = FMath::Clamp(m_fCurrentStamina + m_StaminaRecoveryRate * DeltaSeconds, 0.f, m_MaxStamina);
	}

	// @TODO: Physics objects highlight
	FHitResult outHit;
	FVector vCameraForward = FirstPersonCameraComponent->GetForwardVector();
	FVector vRayStart = GetActorLocation() + FirstPersonCameraComponent->GetRelativeLocation();
	FVector vRayEnd = vRayStart + (vCameraForward * m_MaxGrabDistance);
	UMeshComponent* pHitMesh = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(outHit, vRayStart, vRayEnd, ECC_Visibility))
	{
		UPrimitiveComponent* pHitComponent = outHit.GetComponent();
		if ((pHitComponent != nullptr) && (pHitComponent->Mobility == EComponentMobility::Movable) && (pHitComponent->IsSimulatingPhysics()))
		{
			pHitMesh = Cast<UMeshComponent>(pHitComponent);
		}
	}
	SetHighlightedMesh(pHitMesh);

	// @TODO: Grabbed object update
	if (m_bIsGrabbing)
	{
		m_PhysicsHandle->SetTargetLocation(vRayStart + (vCameraForward * m_fCurrentGrabDistance));
	}
}

void APhysicsCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APhysicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Look);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::GrabObject);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Completed, this, &APhysicsCharacter::ReleaseObject);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APhysicsCharacter::SetIsSprinting(bool NewIsSprinting)
{
	// @TODO: Enable/disable sprinting use CharacterMovementComponent
	if ((!m_bSprintLocked) && (m_bIsSprinting != NewIsSprinting))
	{
		if (!m_bIsSprinting)
		{
      GetCharacterMovement()->MaxWalkSpeed *= m_SprintSpeedMultiplier;
		}
		else
		{
      GetCharacterMovement()->MaxWalkSpeed /= m_SprintSpeedMultiplier;
		}
		m_bIsSprinting = NewIsSprinting;
	}

  m_bSprintLocked = m_bSprintLocked && NewIsSprinting;
}

float APhysicsCharacter::GetCurrentStaminaPercent()
{
	return (m_fCurrentStamina / m_MaxStamina);
}

void APhysicsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APhysicsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APhysicsCharacter::Sprint(const FInputActionValue& Value)
{
	SetIsSprinting(Value.Get<bool>());
}

void APhysicsCharacter::GrabObject(const FInputActionValue& Value)
{
	// @TODO: Grab objects using UPhysicsHandleComponent
	if (!m_bIsGrabbing)
	{
		FHitResult outHit;
		FVector vCameraForward = FirstPersonCameraComponent->GetForwardVector();
		FVector vRayStart = GetActorLocation() + FirstPersonCameraComponent->GetRelativeLocation();
		FVector vRayEnd = vRayStart + (vCameraForward * m_MaxGrabDistance);
		
		if (GetWorld()->LineTraceSingleByChannel(outHit, vRayStart, vRayEnd, ECC_Visibility))
		{
			UPrimitiveComponent* pHitComponent = outHit.GetComponent();
			if ((pHitComponent != nullptr) && (pHitComponent->Mobility == EComponentMobility::Movable))
			{
				if (outHit.GetActor()->ActorHasTag(TEXT("Door")))
				{
					m_PhysicsHandle->GrabComponentAtLocation(pHitComponent, outHit.BoneName, outHit.Location);
				}
				else
				{
					m_PhysicsHandle->GrabComponentAtLocationWithRotation(pHitComponent, outHit.BoneName, outHit.Location, outHit.GetActor()->GetActorRotation());
				}
				m_PhysicsHandle->SetInterpolationSpeed(m_BaseInterpolationSpeed / outHit.GetComponent()->GetMass());
				m_fCurrentGrabDistance = outHit.Distance;
				m_bIsGrabbing = true;
			}
		}
	}
}

void APhysicsCharacter::ReleaseObject(const FInputActionValue& Value)
{
	// @TODO: Release grabebd object using UPhysicsHandleComponent
	if (m_bIsGrabbing)
	{
		m_PhysicsHandle->ReleaseComponent();
		m_bIsGrabbing = false;
	}
}

void APhysicsCharacter::SetHighlightedMesh(UMeshComponent* StaticMesh)
{
	if(m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(nullptr);
	}
	m_HighlightedMesh = StaticMesh;
	if (m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(m_HighlightMaterial);
	}
}
