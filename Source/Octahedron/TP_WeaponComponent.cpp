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
#include "Public/FPAnimInstance.h"
#include "Public/CustomProjectile.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	//BoundsScale = 2.f;

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

	// Try and fire a projectile
	if (IsProjectileWeapon)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			FVector StartVector = IWeaponWielderInterface::Execute_GetTraceStart(WeaponWielder);
			FVector ForwardVector = IWeaponWielderInterface::Execute_GetTraceForward(WeaponWielder);
			float spread = UKismetMathLibrary::MapRangeClamped(ADSAlpha, 0.f, 1.f, MaxSpread, MinSpread);
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
		// Trace from center screen to max weapon range
		FVector StartVector = IWeaponWielderInterface::Execute_GetTraceStart(WeaponWielder);
		FVector ForwardVector = IWeaponWielderInterface::Execute_GetTraceForward(WeaponWielder);
		float spread = UKismetMathLibrary::MapRangeClamped(ADSAlpha, 0.f, 1.f, MaxSpread, MinSpread);
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
}

void UTP_WeaponComponent::StopFire()
{
	RecoilStop();
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

void UTP_WeaponComponent::ADSTLCallback(float val)
{
	IWeaponWielderInterface::Execute_OnADSTLUpdate(WeaponWielder, val);
}

//Call this function when the firing begins, the recoil starts here
void UTP_WeaponComponent::RecoilStart()
{
	if (RecoilCurve)
	{

		//Setting all rotators to default values

		WielderDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
		RecoilDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
		Del = FRotator(0.0f, 0.0f, 0.0f);
		RecoilStartRot = UKismetMathLibrary::NormalizedDeltaRotator(IWeaponWielderInterface::Execute_GetWielderControlRotation(WeaponWielder), FRotator{0.f, 0.f, 0.f}); // in certain angles, the recovery can just cancel itself if we don't delta this with 0

		IsShouldRecoil = true;

		//Timer for the recoil: I have set it to 10s but dependeding how long it takes to empty the gun mag, you can increase the time.
		GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UTP_WeaponComponent::FireTimerFunction, RecoilToStableTime, false);

		GetWorld()->GetTimerManager().SetTimer(RecoilTimer, this, &UTP_WeaponComponent::RecoilTimerCallback, (1.f / 60.f), true);
	}
}

void UTP_WeaponComponent::FireTimerFunction()
{
	GetWorld()->GetTimerManager().PauseTimer(FireTimer);
}

//Called when firing stops
void UTP_WeaponComponent::RecoilStop()
{
	IsShouldRecoil = false;
}

void UTP_WeaponComponent::RecoilTimerCallback()
{
	float recoiltime;
	FVector RecoilVec;

	//Calculation of control rotation to update 
	recoiltime = GetWorld()->GetTimerManager().GetTimerElapsed(FireTimer);
	RecoilVec = RecoilCurve->GetVectorValue(recoiltime);
	Del.Roll = 0;
	Del.Pitch = (RecoilVec.Y);
	Del.Yaw = (RecoilVec.Z);
	WielderDeltaRot = IWeaponWielderInterface::Execute_GetWielderControlRotation(WeaponWielder) - RecoilStartRot - RecoilDeltaRot;
	FRotator newRotator = RecoilStartRot + WielderDeltaRot + Del;
	IWeaponWielderInterface::Execute_SetWielderControlRotation(WeaponWielder, newRotator);
	RecoilDeltaRot = Del;

	//Conditionally start resetting the recoil
	if (!IsShouldRecoil)
	{
		if (recoiltime > FireDelay)
		{
			GetWorld()->GetTimerManager().ClearTimer(RecoilTimer);
			RecoilTimer.Invalidate();

			GetWorld()->GetTimerManager().ClearTimer(FireTimer);
			FireTimer.Invalidate();
			RecoveryStart();
		}
	}
	//UE_LOG(LogTemp, Display, TEXT("deltapitch: %f"), deltaPitch);
}

//This function is automatically called, no need to call this. It is inside the Tick function
void UTP_WeaponComponent::RecoveryStart()
{
	if (IWeaponWielderInterface::Execute_GetWielderControlRotation(WeaponWielder).Pitch > RecoilStartRot.Pitch)
	{
		GetWorld()->GetTimerManager().SetTimer(StopRecoveryTimer, this, &UTP_WeaponComponent::StopRecoveryTimerFunction, RecoveryTime, false);
		GetWorld()->GetTimerManager().SetTimer(RecoilRecoveryTimer, this, &UTP_WeaponComponent::RecoilRecoveryTimerCallback, (1.f / 60.f), true);
	}
}

//This function too is automatically called from the recovery start function.
void UTP_WeaponComponent::StopRecoveryTimerFunction()
{
	GetWorld()->GetTimerManager().ClearTimer(RecoilRecoveryTimer);
	RecoilRecoveryTimer.Invalidate();
	GetWorld()->GetTimerManager().ClearTimer(StopRecoveryTimer);
	StopRecoveryTimer.Invalidate();
}

void UTP_WeaponComponent::RecoilRecoveryTimerCallback()
{
	// servicable for now, but full auto still have a problem where if you move too far from the origin, the recovery is too strong (above the origin) or there won't be any recovery (below origin)
	FVector RecoilVec;

	//Recoil resetting
	FRotator originalRotator = IWeaponWielderInterface::Execute_GetWielderControlRotation(WeaponWielder);
	FRotator tmprot = originalRotator;
	float deltaPitch = UKismetMathLibrary::NormalizedDeltaRotator(tmprot, RecoilStartRot).Pitch;
	if (deltaPitch > 0)
	{
		FRotator TargetRot = originalRotator - RecoilDeltaRot;
		float InterpSpeed = UKismetMathLibrary::MapRangeClamped(deltaPitch, 0.f, MaxRecoilPitch, 3.f, 10.f);
		//UE_LOG(LogTemp, Display, TEXT("interpspeed: %f"), InterpSpeed);
		
		FRotator newRotation = UKismetMathLibrary::RInterpTo(originalRotator, TargetRot, GetWorld()->DeltaTimeSeconds, InterpSpeed);
		IWeaponWielderInterface::Execute_SetWielderControlRotation(WeaponWielder, newRotation);
		RecoilDeltaRot = RecoilDeltaRot + (originalRotator - tmprot);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoilRecoveryTimer);
		RecoilRecoveryTimer.Invalidate();
		GetWorld()->GetTimerManager().ClearTimer(StopRecoveryTimer);
		StopRecoveryTimer.Invalidate();
	}
	//UE_LOG(LogTemp, Display, TEXT("deltapitch: %f"), deltaPitch);
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
