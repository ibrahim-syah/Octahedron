// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "TP_WeaponComponent.generated.h"

class AOctahedronCharacter;
class UTimelineComponent;
class USightMeshComponent;
class UUserWidget;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class OCTAHEDRON_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AOctahedronProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** AnimMontage to play when equipping or picking up the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* EquipAnimation;

	/** AnimMontage to play when reloading the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* ReloadAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FString WeaponName{"Weapon Base"};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool CanFire{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool FireNextShot{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Damage{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float FireDelay{ 0.5f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Range{ 10000.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Spread{ 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	UMaterialParameterCollection* MPC_FP;
	UMaterialParameterCollectionInstance* MPC_FP_Instance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float FOV_Base{ 90.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float FOV_ADS{ 70.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float ADS_Speed{ 0.35f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	FVector ADS_Offset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** ADS Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ADSAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* ADSTL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ADSAlphaCurve;
	float ADSAlpha;

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void ADSTLCallback(float val);

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeapon(AOctahedronCharacter* TargetCharacter);

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	/** Stow the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Stow();

	/** Equip the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Equip();

	/** Reload the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	/** Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ADS();

	/** Release Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ReleaseADS();

	/** force Exit Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ExitADS();

	/** Scope Sight Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	USightMeshComponent* ScopeSightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	UUserWidget* ScopeReticleWidget;

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginPlay();

private:
	/** The Character holding this weapon*/
	AOctahedronCharacter* Character;

	bool IsEquipping;
	UFUNCTION()
	void EquipAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted);
	FTimerHandle EquipDelayTimerHandle;
	void SetIsEquippingFalse();

	bool IsReloading;
	UFUNCTION()
	void ReloadAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted);
	FTimerHandle ReloadDelayTimerHandle;
	void SetIsReloadingFalse();
};
