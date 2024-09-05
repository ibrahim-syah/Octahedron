// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "EFireMode.h"
#include "Public/EAmmoType.h"
#include "TP_WeaponComponent.generated.h"

class UTimelineComponent;
class USightMeshComponent;
class UUserWidget;
class UCurveVector;
class UNiagaraSystem;
struct FInputActionValue;
class UMetaSoundSource;
class UDefaultCameraShakeBase;
class UCameraShakeBase;
class ACustomProjectile;

DECLARE_DELEGATE(FOnWeaponStow);
DECLARE_DELEGATE_OneParam(FOnWeaponChange, UTP_WeaponComponent*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponProjectileFireSignature, FHitResult, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHitScanFireSignature, TArray<FHitResult>, HitResults);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipSignature, AActor*, WeaponWielder, UTP_WeaponComponent*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStowSignature, AActor*, WeaponWielder, UTP_WeaponComponent*, Weapon);


UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class OCTAHEDRON_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	UCurveFloat* RecoilDirectionCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	float RecoilStat = 85.f;

	void StartRecoil();
	bool bIsRecoilActive;

	UPROPERTY(EditAnywhere)
	float BaseRecoilPitchForce = 5.f;
	float InitialRecoilPitchForce;
	float RecoilPitchDamping;
	float RecoilPitchVelocity;

	UPROPERTY(EditAnywhere)
	float BaseRecoilYawForce = 8.f;
	float InitialRecoilYawForce;
	float RecoilYawDamping;
	float RecoilYawVelocity;

	void StartRecoilRecovery();
	bool bIsRecoilRecoveryActive;
	bool bIsRecoilNeutral = true;
	FRotator RecoilCheckpoint;

	bool bUpdateRecoilPitchCheckpointInNextShot = false;

	bool bIsRecoilYawRecoveryActive;
	bool bUpdateRecoilYawCheckpointInNextShot = false;

	float BloomStep = 0.5f;
	UPROPERTY(BlueprintReadOnly, Category = Recoil)
	float CurrentBloom = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Recoil)
	float ADSBloomModifier = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Recoil)
	float BloomRecoveryInterpSpeed = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Recoil)
	float MaxBloom = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Recoil)
	float MaxADSHeat = 10.f;
	float CurrentADSHeat = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	float ADSHeatModifierMax = 0.6;

	/** projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	bool IsProjectileWeapon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UMaterialInstance* FP_Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FTransform TPTransform{};

	///** AnimSequence for the gun mesh to play each time we fire on Weapon*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimSequence* WeaponMeshFireAnimation = nullptr;
	TEnumAsByte<EAnimationMode::Type> DefaultAnimationMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TSubclassOf<UAnimInstance> TPWeaponAnimLinkLayer;

	/** AnimMontage to play when equipping the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* TPEquipAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* TPReloadAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* TPFireAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* TPMeleeAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimSequence* FPIdlePoseAnimation = nullptr;

	/** AnimMontage to play when equipping the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FPEquipAnimation = nullptr;

	/** AnimMontage to play when reloading the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FPReloadAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FPFireAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FPMeleeAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FName MuzzleSocketName{"Muzzle"};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FName ShellEjectSocketName{ "ShellEject" };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FString WeaponName{"Weapon Base"};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool CanFire{ false };

	bool FireNextShot{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Damage{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Range{ 10000.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool CanSwitchFireMode{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	int32 BurstFireRounds{ 3 };
	int32 BurstFireCurrent{ 0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float FireRate{ 560.f }; // in rounds per minute. e.g. 60 RPM means there is a delay of 1 second for every shot
	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
	float FireDelay{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	int32 Pellets{ 1 }; // more than 1 means it's a pellet gun (shotgun)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float PelletSpread{ 1.f }; // spread of each individual pellet is originalspread + (n/PelletSpread)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	EFireMode FireMode{ EFireMode::Single };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	UMaterialParameterCollection* MPC_FP = nullptr;
	UMaterialParameterCollectionInstance* MPC_FP_Instance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float Sight_ForwardLength{ 30.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float FOV_Base{ 90.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float FOV_ADS{ 70.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ADS)
	float ADS_Speed{ 0.35f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* ADSTL = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ADSAlphaCurve = nullptr;
	float ADSAlpha{ 0.f };
	float ADSAlphaLerp{ 1.f };

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void ADSTLCallback(float val);

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	// weapon fire effect (Shell eject, muzzle flash, tracer) implemented in blueprint but called from cpp (Fire() function)
	UPROPERTY(BlueprintAssignable)
	FOnWeaponHitScanFireSignature OnWeaponHitScanFireDelegate;

	// weapon projectile fire effect (no hit result passed)
	UPROPERTY(BlueprintAssignable)
	FOnWeaponProjectileFireSignature OnWeaponProjectileFireDelegate;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float EquipTime{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	USoundBase* EquipSound = nullptr;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnEquipSignature OnEquipDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnStowSignature OnStowDelegate;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchFireMode();

	/** Reload the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CancelReload(float BlendTime);

	/** force Exit Aim down sight */
	UFUNCTION(BlueprintCallable, Category = "ADS")
	void ExitADS(bool IsFast);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	bool ADS_Held;

	/** Scope Sight Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	USightMeshComponent* ScopeSightMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	TSubclassOf<UUserWidget> ScopeReticleWidget = nullptr;

	UFUNCTION(BlueprintPure)
	bool GetIsReloading() const { return IsReloading; };


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	TSubclassOf<UCameraShakeBase> FireCamShake = nullptr;

	FTimerHandle FireTimer;
	void FireTimerFunction();

	// Weapon Mesh Recoil/Kick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FVector RecoilLocMin{ -0.1f, -3.f, 0.2f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FVector RecoilLocMax{ 0.1f, -1.f, 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FRotator RecoilRotMin{ -5.f, -1.f, -3.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FRotator RecoilRotMax{ 5.f, 1.f, -1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FVector RecoilLocMinADS{ 0.f, -7.f, 0.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FVector RecoilLocMaxADS{ 0.f, -6.f, 0.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FRotator RecoilRotMinADS{ 0.f, 0.f, 0.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	FRotator RecoilRotMaxADS{ 0.f, 0.f, 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	float RecoilKickInterpSpeedScale = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Recoil)
	float RecoilRecoveryInterpSpeedScale = 36.f;

	// Effects
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* MuzzleFlash_FX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* Tracer_FX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* ShellEject_FX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	UStaticMesh* ShellEjectMesh = nullptr;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	APawn* GetOwningWeaponWielder() { return WeaponWielder; }

	UFUNCTION(BlueprintCallable)
	void SetOwningWeaponWielder(APawn* newWeaponWielder);

	// SFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX, meta = (AllowPrivateAccess = "true"))
	UMetaSoundSource* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SFX)
	float FireSoundDelayScale{ 0.5f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cosmetics)
	float WeaponSwaySpeed{ 10.f }; // determine how heavy the weapon is for weapon sway speed, larger means faster. clamped at [6, 80]

	UFUNCTION(BlueprintCallable)
	void SetIsEquippingFalse();

	UFUNCTION(BlueprintCallable)
	void SetIsStowingFalse();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsEquipping;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsStowing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsReloading;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsWielderHoldingShootButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTimerHandle FireRateDelayTimerHandle;

	//UPROPERTY(BlueprintAssignable)
	FOnWeaponChange WeaponChangeDelegate;

	//UPROPERTY(BlueprintAssignable)
	FOnWeaponStow WeaponStowDelegate;


	UFUNCTION(BlueprintCallable)
	void SingleFire();

	UFUNCTION(BlueprintCallable)
	void BurstFire();

	UFUNCTION(BlueprintCallable)
	void FullAutoFire();

	UFUNCTION(BlueprintCallable)
	void EquipAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted);

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginPlay();

private:
	// The Pawn holding this weapon
	// The pawn needs to implement the WeaponWielderInterface
	APawn* WeaponWielder = nullptr;

	FTimerHandle EquipDelayTimerHandle;

	FTimerHandle ReloadDelayTimerHandle;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	EAmmoType AmmoType{ EAmmoType::Primary };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
	int32 MaxMagazineCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	int32 CurrentMagazineCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	USoundBase* DryFireSound = nullptr;

	UFUNCTION(BlueprintCallable)
	void OnReloaded();

	UFUNCTION(BlueprintCallable)
	void SetIsReloadingFalse();
};
