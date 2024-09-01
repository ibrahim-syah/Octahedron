// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "Weapon/WeaponWielderInterface.h"
#include "OctahedronProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Blueprint/UserWidget.h"
#include "SightMeshComponent.h"
#include "DefaultCameraShakeBase.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"
#include "Public/FPAnimInstance.h"
#include "Public/CustomProjectile.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	BoundsScale = 2.f;

	ADSTL = CreateDefaultSubobject<UTimelineComponent>(FName("ADSTL"));
	ADSTL->SetTimelineLength(1.f);
	ADSTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
}

void UTP_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ADSAlphaCurve != nullptr)
	{
		FOnTimelineFloat onADSTLCallback;
		onADSTLCallback.BindUFunction(this, FName{ TEXT("ADSTLCallback") });
		ADSTL->AddInterpFloat(ADSAlphaCurve, onADSTLCallback);
	}

	if (ScopeSightMesh != nullptr)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		ScopeSightMesh->AttachToComponent(this, AttachmentRules, FName(TEXT("Sight")));
	}

	if (IsValid(MPC_FP))
	{
		MPC_FP_Instance = GetWorld()->GetParameterCollectionInstance(MPC_FP);
	}

	FireDelay = 60.f / FireRate;
	CurrentMagazineCount = MaxMagazineCount;
	DefaultAnimationMode = GetAnimationMode();
}

void UTP_WeaponComponent::SetOwningWeaponWielder(APawn* newWeaponWielder)
{
	if (newWeaponWielder && newWeaponWielder->GetClass()->ImplementsInterface(UWeaponWielderInterface::StaticClass()))
	{
		WeaponWielder = newWeaponWielder;
	}
}

void UTP_WeaponComponent::SwitchFireMode()
{
	switch (FireMode)
	{
	case EFireMode::Single:
		FireMode = EFireMode::Burst;
		break;
	case EFireMode::Burst:
		FireMode = EFireMode::Auto;
		break;
	case EFireMode::Auto:
		FireMode = EFireMode::Single;
		break;
	default:
		break;
	}
}

void UTP_WeaponComponent::SingleFire()
{
	Fire();

	StopFire();
}

void UTP_WeaponComponent::BurstFire()
{
	Fire();
	BurstFireCurrent++;
	if (BurstFireCurrent >= BurstFireRounds)
	{
		// Ensure the timer is cleared by using the timer handle
		GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
		FireRateDelayTimerHandle.Invalidate();

		GetWorld()->GetTimerManager().SetTimer(FireRateDelayTimerHandle, FireDelay, false);
		BurstFireCurrent = 0;
		StopFire();
	}
}

void UTP_WeaponComponent::FullAutoFire()
{
	Fire();
	if (!IsWielderHoldingShootButton)
	{
		GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
		FireRateDelayTimerHandle.Invalidate();

		GetWorld()->GetTimerManager().SetTimer(FireRateDelayTimerHandle, FireDelay, false);
		StopFire();
	}
}

void UTP_WeaponComponent::Fire()
{
	if (!CanFire || IsReloading || IsEquipping || IsStowing || GetWorld()->GetTimerManager().GetTimerRemaining(FireRateDelayTimerHandle) > 0)
	{
		return;
	}

	if (CurrentMagazineCount <= 0)
	{
		StopFire();
		IsWielderHoldingShootButton = false;
		if (IsValid(DryFireSound))
		{
			UGameplayStatics::SpawnSoundAttached(
				DryFireSound,
				this
			);
		}

		if (IWeaponWielderInterface::Execute_GetRemainingAmmo(WeaponWielder) > 0)
		{
			Reload();
			return;
		}
		return;
	}

	// Trace from center screen to max weapon range
	FVector StartVector = IWeaponWielderInterface::Execute_GetTraceStart(WeaponWielder);
	FVector ForwardVector = IWeaponWielderInterface::Execute_GetTraceForward(WeaponWielder);
	float spread = UKismetMathLibrary::MapRangeClamped(ADSAlpha, 0.f, 1.f, MaxSpread, MinSpread);

	// Try and fire a projectile
	if (IsProjectileWeapon)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			FHitResult MuzzleTraceResult;

			FVector RandomDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardVector, spread + (1 / PelletSpread));
			FVector ResultingVector = RandomDirection * Range;
			FVector EndVector = StartVector + ResultingVector;

			FHitResult WielderTraceResult{};
			FCollisionQueryParams Params = FCollisionQueryParams();
			Params.AddIgnoredActor(GetOwner());
			Params.AddIgnoredActor(WeaponWielder);
			bool isHit = GetWorld()->LineTraceSingleByChannel(
				WielderTraceResult,
				StartVector,
				EndVector,
				ECollisionChannel::ECC_GameTraceChannel2,
				Params
			);

			// Trace from weapon muzzle to center trace hit location

			FVector EndTrace{};
			if (isHit)
			{
				FVector ScaledDirection = RandomDirection * 10.f;
				EndTrace = WielderTraceResult.Location + ScaledDirection;
			}
			else
			{
				EndTrace = WielderTraceResult.TraceEnd;
			}

			Params.bReturnPhysicalMaterial = true;
			GetWorld()->LineTraceSingleByChannel(
				MuzzleTraceResult,
				GetSocketLocation(MuzzleSocketName),
				EndTrace,
				ECollisionChannel::ECC_GameTraceChannel2,
				Params
			);

			CurrentMagazineCount = FMath::Max(CurrentMagazineCount - 1, 0);
			OnWeaponProjectileFireDelegate.Broadcast(MuzzleTraceResult); // projectile is spawned in bp
		}
	}
	else // hitscan weapon
	{
		TArray<FHitResult> MuzzleTraceResults;
		for (int32 i = 0; i < Pellets; i++) // bruh idk if this is a good idea, but whatever man
		{
			FVector RandomDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardVector, spread + (i / PelletSpread));
			FVector ResultingVector = RandomDirection * Range;
			FVector EndVector = StartVector + ResultingVector;

			FHitResult WielderTraceResult{};
			FCollisionQueryParams Params = FCollisionQueryParams();
			Params.AddIgnoredActor(GetOwner());
			Params.AddIgnoredActor(WeaponWielder);
			bool isHit = GetWorld()->LineTraceSingleByChannel(
				WielderTraceResult,
				StartVector,
				EndVector,
				ECollisionChannel::ECC_GameTraceChannel2,
				Params
			);

			// Trace from weapon muzzle to center trace hit location

			FVector EndTrace{};
			if (isHit)
			{
				FVector ScaledDirection = RandomDirection * 10.f;
				EndTrace = WielderTraceResult.Location + ScaledDirection;
			}
			else
			{
				EndTrace = WielderTraceResult.TraceEnd;
			}

			Params.bReturnPhysicalMaterial = true;
			FHitResult MuzzleTraceResult{};
			GetWorld()->LineTraceSingleByChannel(
				MuzzleTraceResult,
				GetSocketLocation(MuzzleSocketName),
				EndTrace,
				ECollisionChannel::ECC_GameTraceChannel2,
				Params
			);
			MuzzleTraceResults.Add(MuzzleTraceResult);
		}

		CurrentMagazineCount = FMath::Max(CurrentMagazineCount - 1, 0);
		OnWeaponHitScanFireDelegate.Broadcast(MuzzleTraceResults);
	}

	// Try and play a firing animation for the weapon mesh if specified
	if (WeaponMeshFireAnimation != nullptr)
	{
		SetAnimationMode(EAnimationMode::AnimationSingleNode);
		PlayAnimation(WeaponMeshFireAnimation, false);
	}

	IWeaponWielderInterface::Execute_OnWeaponFired(WeaponWielder);

	if (bIsRecoilNeutral)
	{
		RecoilCheckpoint = WeaponWielder->GetControlRotation();
		bIsRecoilNeutral = false;
	}
	if (bUpdateRecoilPitchCheckpointInNextShot)
	{
		RecoilCheckpoint = FRotator(WeaponWielder->GetControlRotation().Pitch, RecoilCheckpoint.Yaw, RecoilCheckpoint.Roll);
		bUpdateRecoilPitchCheckpointInNextShot = false;
	}
	if (bUpdateRecoilYawCheckpointInNextShot)
	{
		RecoilCheckpoint = FRotator(RecoilCheckpoint.Pitch, WeaponWielder->GetControlRotation().Yaw, RecoilCheckpoint.Roll);
		bUpdateRecoilYawCheckpointInNextShot = false;
	}
	StartRecoil();
}

void UTP_WeaponComponent::StopFire()
{
	IWeaponWielderInterface::Execute_OnStopFiring(WeaponWielder);
}

void UTP_WeaponComponent::ForceStopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
	FireRateDelayTimerHandle.Invalidate();
	IsWielderHoldingShootButton = false; // force ai controller to stop firing full auto, idk if it's a good solution, but def the quickest
	StopFire();
}

void UTP_WeaponComponent::Stow()
{

	if (IsValid(EquipSound))
	{
		UGameplayStatics::SpawnSoundAttached(
			EquipSound,
			this
		);
	}
	GetWorld()->GetTimerManager().SetTimer(EquipDelayTimerHandle, this, &UTP_WeaponComponent::SetIsStowingFalse, EquipTime, false);
}

void UTP_WeaponComponent::Equip()
{

	if (IsValid(EquipSound))
	{
		UGameplayStatics::SpawnSoundAttached(
			EquipSound,
			this
		);
	}
}

void UTP_WeaponComponent::EquipAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted)
{
	SetIsEquippingFalse();
}

void UTP_WeaponComponent::SetIsEquippingFalse()
{
	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(EquipDelayTimerHandle);
	EquipDelayTimerHandle.Invalidate();

	IsEquipping = false;
}

void UTP_WeaponComponent::SetIsStowingFalse()
{
	// Detach the weapon from the First Person Character
	FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepRelative, true);
	DetachFromComponent(DetachmentRules);

	CanFire = false;
	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(EquipDelayTimerHandle);
	EquipDelayTimerHandle.Invalidate();

	IsStowing = false;
	OnStowDelegate.Broadcast(WeaponWielder, this);
}

void UTP_WeaponComponent::OnReloaded()
{
	int toBeLoaded = FMath::Min(IWeaponWielderInterface::Execute_GetRemainingAmmo(WeaponWielder), MaxMagazineCount);
	int newValue = FMath::Max(IWeaponWielderInterface::Execute_GetRemainingAmmo(WeaponWielder) - toBeLoaded, 0);
	IWeaponWielderInterface::Execute_SetRemainingAmmo(WeaponWielder, newValue);
	CurrentMagazineCount = FMath::Clamp(toBeLoaded, 0, MaxMagazineCount);
}

void UTP_WeaponComponent::Reload()
{
	if (IsReloading || IsEquipping || IsStowing)
	{
		return;
	}
	if (IWeaponWielderInterface::Execute_GetRemainingAmmo(WeaponWielder) <= 0 || CurrentMagazineCount >= MaxMagazineCount)
	{
		return;
	}
	IsReloading = true;

	ExitADS(false);
	IWeaponWielderInterface::Execute_OnWeaponReload(WeaponWielder);
}

void UTP_WeaponComponent::CancelReload(float BlendTime)
{
	SetIsReloadingFalse();
	IWeaponWielderInterface::Execute_OnWeaponStopReloadAnimation(WeaponWielder, BlendTime);
}

void UTP_WeaponComponent::ExitADS(bool IsFast)
{
	if (IsFast)
	{
		ADSTL->SetPlayRate(2.f);
	}
	ADSTL->Reverse();
}

void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRecoilActive)
	{
		// Apply the current recoil velocity
		IWeaponWielderInterface::Execute_AddWielderControlRotation(WeaponWielder, (RecoilPitchVelocity * DeltaTime) * -1.f, (RecoilYawVelocity * DeltaTime));

		// Decrease the recoil velocity over time
		RecoilPitchVelocity -= RecoilPitchDamping * DeltaTime;
		RecoilYawVelocity -= RecoilYawDamping * DeltaTime;


		if (RecoilPitchVelocity <= 0.0f)
		{
			bIsRecoilActive = false;
			StartRecoilRecovery();
		}
	}
	else if (bIsRecoilRecoveryActive)
	{
		FRotator currentControlRotator = WeaponWielder->GetControlRotation();

		FRotator deltaRot = currentControlRotator - RecoilCheckpoint;
		deltaRot.Normalize();
		if (FMath::Abs(deltaRot.Pitch) < 0.1f)
		{
			bIsRecoilRecoveryActive = false;
			bIsRecoilYawRecoveryActive = false;
			bIsRecoilNeutral = true;
		}
		else
		{
			FRotator interpRot = FMath::RInterpTo(currentControlRotator, RecoilCheckpoint, DeltaTime, 2.f);
			if (!bIsRecoilYawRecoveryActive)
			{
				interpRot.Yaw = currentControlRotator.Yaw;
			}

			IWeaponWielderInterface::Execute_SetWielderControlRotation(WeaponWielder, interpRot);
		}
	}
}

void UTP_WeaponComponent::ADSTLCallback(float val)
{
	IWeaponWielderInterface::Execute_OnADSTLUpdate(WeaponWielder, val);
}

void UTP_WeaponComponent::FireTimerFunction()
{
	GetWorld()->GetTimerManager().PauseTimer(FireTimer);
}

void UTP_WeaponComponent::StartRecoil()
{
	// may want to add skill or stat modifier here to constrict or enlarge the recoil,
	// but we need to figure out how to proportionally scale the damping to keep the frametime consiste
	InitialRecoilPitchForce = BaseRecoilPitchForce;
	RecoilPitchVelocity = InitialRecoilPitchForce;
	RecoilPitchDamping = RecoilPitchVelocity / 0.1f;

	InitialRecoilYawForce = BaseRecoilYawForce;
	RecoilYawVelocity = FMath::RandRange(InitialRecoilYawForce * -1.f, InitialRecoilYawForce);
	RecoilYawDamping = (RecoilYawVelocity >= 0.f ? BaseRecoilYawDamping : BaseRecoilYawDamping * -1.f) / 0.1f;

	bIsRecoilActive = true;
}

void UTP_WeaponComponent::StartRecoilRecovery()
{
	bIsRecoilRecoveryActive = true;
	bIsRecoilYawRecoveryActive = true;
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	/*if (IsValid(WeaponWielder))
	{
		IWeaponWielderInterface::Execute_OnFinishPlay(WeaponWielder);
	}*/
}

void UTP_WeaponComponent::SetIsReloadingFalse()
{
	IsReloading = false;
	IWeaponWielderInterface::Execute_OnSetIsReloadingFalse(WeaponWielder);

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(ReloadDelayTimerHandle);
	ReloadDelayTimerHandle.Invalidate();
}
