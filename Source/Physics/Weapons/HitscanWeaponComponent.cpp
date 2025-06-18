// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/HitscanWeaponComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>
#include "PhysicsCharacter.h"
#include "PhysicsWeaponComponent.h"
#include <Camera/CameraComponent.h>
#include <Components/SphereComponent.h>

void UHitscanWeaponComponent::Fire()
{
	Super::Fire();

	// @TODO: Add firing functionality
	FHitResult outHit;
	FVector vCameraForward = Character->FirstPersonCameraComponent->GetForwardVector();
	FVector vRayStart = Character->GetActorLocation() + Character->FirstPersonCameraComponent->GetRelativeLocation();
	FVector vRayEnd = vRayStart + (vCameraForward * m_fRange);
	if (GetWorld()->LineTraceSingleByChannel(outHit, vRayStart, vRayEnd, ECC_Visibility))
	{
		ApplyDamage(outHit);
		onHitscanImpact.Broadcast(outHit.GetActor(), outHit.ImpactPoint, vCameraForward);
	}
}
