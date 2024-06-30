// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "OctahedronCharacter.h"
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
#include "WeaponFX.h"
#include "WeaponDecals.h"
#include "WeaponImpacts.h"
#include "WeaponSounds.h"
#include "DefaultCameraShakeBase.h"
#include "Curves/CurveVector.h"
#include "Public/FPAnimInstance.h"
#include "Public/CustomProjectile.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{

	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
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
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Failed to find ads curve for this weapon"));
	}

	if (ScopeSightMesh != nullptr)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		ScopeSightMesh->AttachToComponent(this, AttachmentRules, FName(TEXT("Sight")));
	}

	MPC_FP_Instance = GetWorld()->GetParameterCollectionInstance(MPC_FP);

	FireDelay = 60.f / FireRate;

	CurrentMagazineCount = MaxMagazineCount;
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

	//// Ensure the timer is cleared by using the timer handle
	//GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
	//FireRateDelayTimerHandle.Invalidate();
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


		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, FireDelay, false);
		BurstFireCurrent = 0;
		StopFire();
	}
}

void UTP_WeaponComponent::FullAutoFire()
{
	Fire();
	if (!IsPlayerHoldingShootButton)
	{
		GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
		FireRateDelayTimerHandle.Invalidate();

		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, FireDelay, false);
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
		IsPlayerHoldingShootButton = false;
		if (IsValid(DryFireSound))
		{
			UGameplayStatics::SpawnSoundAttached(
				DryFireSound,
				this
			);
		}

		if (Character->GetRemainingAmmo(AmmoType) > 0)
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
			// Trace from center screen to max weapon range
			UCameraComponent* Camera = Character->GetFirstPersonCameraComponent();
			FVector StartVector = Camera->GetComponentLocation();
			FVector ForwardVector = Camera->GetForwardVector();
			float spread = UKismetMathLibrary::MapRangeClamped(ADSAlpha, 0.f, 1.f, MaxSpread, MinSpread);
			FHitResult MuzzleTraceResult;

			FVector RandomDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardVector, spread + (1 / PelletSpread));
			FVector ResultingVector = RandomDirection * Range;
			FVector EndVector = StartVector + ResultingVector;

			FHitResult CameraTraceResult{};
			FCollisionQueryParams Params = FCollisionQueryParams();
			Params.AddIgnoredActor(GetOwner());
			Params.AddIgnoredActor(Character);
			bool isHit = GetWorld()->LineTraceSingleByChannel(
				CameraTraceResult,
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
				EndTrace = CameraTraceResult.Location + ScaledDirection;
			}
			else
			{
				EndTrace = CameraTraceResult.TraceEnd;
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
		UCameraComponent* Camera = Character->GetFirstPersonCameraComponent();
		FVector StartVector = Camera->GetComponentLocation();
		FVector ForwardVector = Camera->GetForwardVector();
		float spread = UKismetMathLibrary::MapRangeClamped(ADSAlpha, 0.f, 1.f, MaxSpread, MinSpread);
		TArray<FHitResult> MuzzleTraceResults;
		for (int32 i = 0; i < Pellets; i++) // bruh idk if this is a good idea, but whatever man
		{
			FVector RandomDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardVector, spread + (i / PelletSpread));
			FVector ResultingVector = RandomDirection * Range;
			FVector EndVector = StartVector + ResultingVector;

			FHitResult CameraTraceResult{};
			FCollisionQueryParams Params = FCollisionQueryParams();
			Params.AddIgnoredActor(GetOwner());
			Params.AddIgnoredActor(Character);
			bool isHit = GetWorld()->LineTraceSingleByChannel(
				CameraTraceResult,
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
				EndTrace = CameraTraceResult.Location + ScaledDirection;
			}
			else
			{
				EndTrace = CameraTraceResult.TraceEnd;
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

	WeaponFireAnimateDelegate.ExecuteIfBound();

	if (IsValid(FireCamShake))
	{
		Character->GetLocalViewingPlayerController()->ClientStartCameraShake(FireCamShake);
	}

	// Try and play a firing animation if specified
	if (WeaponFireAnimation != nullptr)
	{
		PlayAnimation(WeaponFireAnimation, false);
	}

	Character->MakeNoise(1.f, Character, GetComponentLocation());
}

void UTP_WeaponComponent::StopFire()
{
	RecoilStop();
}

void UTP_WeaponComponent::ForceStopFire()
{
	StopFire();
}

void UTP_WeaponComponent::Stow()
{
	Character->SetCurrentWeapon(nullptr);

	WeaponChangeDelegate.BindUFunction(Cast<UFPAnimInstance>(Character->GetMesh1P()->GetAnimInstance()), FName("StowCurrentWeapon"));
	WeaponChangeDelegate.Execute(this);

	if (IsValid(EquipSound))
	{
		UGameplayStatics::SpawnSoundAttached(
			EquipSound,
			this
		);
	}
	Character->GetWorldTimerManager().SetTimer(EquipDelayTimerHandle, this, &UTP_WeaponComponent::SetIsStowingFalse, EquipTime, false);
}

void UTP_WeaponComponent::Equip()
{
	WeaponChangeDelegate.BindUFunction(Cast<UFPAnimInstance>(Character->GetMesh1P()->GetAnimInstance()), FName("SetCurrentWeapon"));
	WeaponChangeDelegate.Execute(this);

	WeaponFireAnimateDelegate.BindUFunction(Cast<UFPAnimInstance>(Character->GetMesh1P()->GetAnimInstance()), FName("Fire"));

	if (IsValid(EquipSound))
	{
		UGameplayStatics::SpawnSoundAttached(
			EquipSound,
			this
		);
	}
	if (Character != nullptr && ReloadAnimation != nullptr)
	{
		if (Character->GetFPAnimInstance())
		{
			Character->GetFPAnimInstance()->Montage_Play(EquipAnimation, 1.f);

			FOnMontageBlendingOutStarted BlendOutDelegate;
			BlendOutDelegate.BindUObject(this, &UTP_WeaponComponent::EquipAnimationBlendOut);
			Character->GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, EquipAnimation);
		}
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

	Character->SetHasWeapon(false);

	Character->RemoveWeaponInputMapping();

	CanFire = false;
	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(EquipDelayTimerHandle);
	EquipDelayTimerHandle.Invalidate();

	IsStowing = false;
	OnStowDelegate.Broadcast(Character, this);
}

void UTP_WeaponComponent::OnReloaded()
{
	int toBeLoaded = FMath::Min(Character->GetRemainingAmmo(AmmoType), MaxMagazineCount);
	int newValue = FMath::Max(Character->GetRemainingAmmo(AmmoType) - toBeLoaded, 0);
	Character->SetRemainingAmmo(AmmoType, newValue);
	CurrentMagazineCount = FMath::Clamp(toBeLoaded, 0, MaxMagazineCount);
}

void UTP_WeaponComponent::Reload()
{
	if (IsReloading || IsEquipping || IsStowing)
	{
		return;
	}
	if (Character->GetRemainingAmmo(AmmoType) <= 0 || CurrentMagazineCount >= MaxMagazineCount)
	{
		return;
	}
	IsReloading = true;

	ExitADS(false);

	if (Character != nullptr && ReloadAnimation != nullptr)
	{
		if (Character->GetFPAnimInstance())
		{
			Character->GetFPAnimInstance()->IsLeftHandIKActive = false;
			Character->GetFPAnimInstance()->Montage_Play(ReloadAnimation, 1.f);

			// remove this blend out logic and just call OnReloaded from the anim notify whatever
			/*FOnMontageBlendingOutStarted BlendOutDelegate;
			BlendOutDelegate.BindUObject(this, &UTP_WeaponComponent::ReloadAnimationBlendOut);
			Character->GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, ReloadAnimation);*/
		}
	}
}

void UTP_WeaponComponent::CancelReload(float BlendTime)
{
	SetIsReloadingFalse();
	if (Character != nullptr && ReloadAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Stop(BlendTime, ReloadAnimation);

			Stop();
		}
	}
}

void UTP_WeaponComponent::EnterADS()
{
	if (IsReloading || IsEquipping || IsStowing)
	{
		return;
	}
	if (Character->GetFPAnimInstance()->Montage_IsPlaying(ReloadAnimation))
	{
		Character->GetFPAnimInstance()->Montage_Stop(0.f, ReloadAnimation);
		Stop();
	}
	
	ADSTL->SetPlayRate(FMath::Clamp(ADS_Speed, 0.1f, 10.f));

	Character->GetFPAnimInstance()->SetSprintBlendOutTime(0.25f);
	Character->ForceStopSprint();
	ADSTL->Play();
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
	if (Character == nullptr && MPC_FP == nullptr)
	{
		return;
	}
	
	ADSAlpha = val;
	ADSAlphaLerp = FMath::Lerp(0.2f, 1.f, (1.f - ADSAlpha));
	Character->ADSAlpha = ADSAlpha;
	float lerpedFOV = FMath::Lerp(FOV_Base, FOV_ADS, ADSAlpha);
	UCameraComponent* camera = Character->GetFirstPersonCameraComponent();
	camera->SetFieldOfView(lerpedFOV);
	float lerpedIntensity = FMath::Lerp(0.4f, 0.7f, ADSAlpha);
	camera->PostProcessSettings.VignetteIntensity = lerpedIntensity;
	float lerpedFlatFov = FMath::Lerp(90.f, 25.f, ADSAlpha);
	MPC_FP_Instance->SetScalarParameterValue(FName("FOV"), lerpedFlatFov);
	FLinearColor OutColor;
	MPC_FP_Instance->GetVectorParameterValue(FName("Offset"), OutColor);
	float lerpedB = FMath::Lerp(0.f, 30.f, ADSAlpha);
	FLinearColor newColor = FLinearColor(OutColor.R, OutColor.G, lerpedB, OutColor.A);
	MPC_FP_Instance->SetVectorParameterValue(FName("Offset"), newColor);

	float newSpeedMultiplier = FMath::Clamp(ADSAlphaLerp, 0.5f, 1);
	Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetBaseWalkSpeed() * newSpeedMultiplier;
}

//Call this function when the firing begins, the recoil starts here
void UTP_WeaponComponent::RecoilStart()
{
	if (RecoilCurve)
	{

		//Setting all rotators to default values

		PlayerDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
		RecoilDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
		Del = FRotator(0.0f, 0.0f, 0.0f);
		RecoilStartRot = UKismetMathLibrary::NormalizedDeltaRotator(PCRef->GetControlRotation(), FRotator{0.f, 0.f, 0.f}); // in certain angles, the recovery can just cancel itself if we don't delta this with 0

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
	PlayerDeltaRot = PCRef->GetControlRotation() - RecoilStartRot - RecoilDeltaRot;
	PCRef->SetControlRotation(RecoilStartRot + PlayerDeltaRot + Del);
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
	if (PCRef->GetControlRotation().Pitch > RecoilStartRot.Pitch)
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
	FRotator tmprot = PCRef->GetControlRotation();
	float deltaPitch = UKismetMathLibrary::NormalizedDeltaRotator(tmprot, RecoilStartRot).Pitch;
	if (deltaPitch > 0)
	{
		FRotator TargetRot = PCRef->GetControlRotation() - RecoilDeltaRot;
		float InterpSpeed = UKismetMathLibrary::MapRangeClamped(deltaPitch, 0.f, MaxRecoilPitch, 3.f, 10.f);
		//UE_LOG(LogTemp, Display, TEXT("interpspeed: %f"), InterpSpeed);
		
		PCRef->SetControlRotation(UKismetMathLibrary::RInterpTo(PCRef->GetControlRotation(), TargetRot, GetWorld()->DeltaTimeSeconds, InterpSpeed));
		RecoilDeltaRot = RecoilDeltaRot + (PCRef->GetControlRotation() - tmprot);
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
	if (Character == nullptr)
	{
		return;
	}

	Character->RemoveWeaponInputMapping();
}

void UTP_WeaponComponent::SetIsReloadingFalse()
{
	IsReloading = false;
	Character->GetFPAnimInstance()->IsLeftHandIKActive = true;

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(ReloadDelayTimerHandle);
	ReloadDelayTimerHandle.Invalidate();
}
