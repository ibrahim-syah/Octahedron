// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "EFireMode.h"
#include "TP_WeaponComponent.generated.h"

class AOctahedronCharacter;
class UTimelineComponent;
class USightMeshComponent;
class UUserWidget;
struct FInputActionValue;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipSignature, AOctahedronCharacter*, Character, UTP_WeaponComponent*, Weapon);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class OCTAHEDRON_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AOctahedronProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UMaterialInstance* FP_Material;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimSequence* IdlePose;
	
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool CanSwitchFireMode{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	int BurstFireRounds{ 3 };
	int BurstFireCurrent{ 0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float FireRate{ 560.f }; // in rounds per minute. e.g. 60 RPM means there is a delay of 1 second for every shot

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	EFireMode FireMode{ EFireMode::Single };

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchFireModeAction;

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

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceStopFire();

	/** Stow the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Stow();

	/** Equip the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Equip();

	UPROPERTY(BlueprintAssignable)
	FOnEquipSignature OnEquipDelegate;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchFireMode();

	/** Reload the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CancelReload();

	/** Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void PressedADS();

	UFUNCTION(BlueprintCallable, Category = "ADS")
	void EnterADS();

	/** Release Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ReleasedADS();

	/** force Exit Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ExitADS(bool IsFast);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	bool ADS_Held;

	/** Scope Sight Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	USightMeshComponent* ScopeSightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	UUserWidget* ScopeReticleWidget;

	UFUNCTION(BlueprintPure)
	bool GetIsReloading() const { return IsReloading; };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* RecoilTL;

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void RecoilTLUpdateEvent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float Recoil_Speed{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float RecoilReversePlayRate{ 13.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float RecoilPitchReversePlayRate{ 13.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float RecoilYawReversePlayRate{ 3.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* RecoilPitchCurve;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void RecoilPitchTLCallback(float val);
	float RecoilPitch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* RecoilYawCurve;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void RecoilYawTLCallback(float val);
	float RecoilYaw;

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginPlay();

	void PressedFire();
	void ReleasedFire();

	void PressedReload();

	void PressedSwitchFireMode();

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

	FTimerHandle FireRateDelayTimerHandle;
	bool IsPlayerHoldingShootButton;
	UFUNCTION()
	void BurstFire();

	UFUNCTION()
	void FullAutoFire();
};
