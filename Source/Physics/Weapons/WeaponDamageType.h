#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "WeaponDamageType.generated.h"

UENUM(BlueprintType)
enum class EImpulseType : uint8
{
	Linear UMETA(DisplayName = "Linear"),
	Radial UMETA(DisplayName = "Radial"),
	None UMETA(DisplayName = "None")
};

UCLASS(Blueprintable, EditInlineNew)
class PHYSICS_API UWeaponDamageType : public UObject
{
	GENERATED_BODY()

public:
	/** @TODO: Create damage data object */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	float m_fDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	float m_fExplosionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	EImpulseType m_eImpulseType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> m_oDamageTypeClass;
};
