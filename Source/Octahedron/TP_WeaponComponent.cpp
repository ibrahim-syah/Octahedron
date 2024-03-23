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
}

void UTP_WeaponComponent::PressedFire()
{
	IsPlayerHoldingShootButton = true;
	if (Character == nullptr || PCRef == nullptr || !Character->CanAct() || GetWorld()->GetTimerManager().GetTimerRemaining(FireRateDelayTimerHandle) > 0)
	{
		return;
	}

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
	FireRateDelayTimerHandle.Invalidate();
	RecoilStart();
	switch (FireMode)
	{
	case EFireMode::Single:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::SingleFire, FireDelay, true, 0.f);
		break;
	case EFireMode::Burst:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::BurstFire, FireDelay, true, 0.f);
		break;
	case EFireMode::Auto:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::FullAutoFire, FireDelay, true, 0.f);
		break;
	default:
		break;
	}
}

void UTP_WeaponComponent::ReleasedFire()
{
	IsPlayerHoldingShootButton = false;
}

void UTP_WeaponComponent::PressedReload()
{
	if (Character == nullptr || PCRef == nullptr || !Character->CanAct())
	{
		return;
	}

	Reload();
}

void UTP_WeaponComponent::PressedSwitchFireMode()
{
	if (!CanSwitchFireMode || Character == nullptr || PCRef == nullptr || !Character->CanAct())
	{
		return;
	}

	SwitchFireMode();
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

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
	FireRateDelayTimerHandle.Invalidate();
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

		StopFire();
	}
}

void UTP_WeaponComponent::Fire()
{
	if (IsReloading || IsEquipping || GetWorld()->GetTimerManager().GetTimerRemaining(FireRateDelayTimerHandle) > 0)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const FRotator SpawnRotation = PCRef->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			World->SpawnActor<AOctahedronProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
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
			Params.AddIgnoredActor(Character);
			bool isHit = GetWorld()->LineTraceSingleByChannel(
				CameraTraceResult,
				StartVector,
				EndVector,
				ECollisionChannel::ECC_Visibility,
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

			//const FName TraceTag("MyTraceTag");
			//GetWorld()->DebugDrawTraceTag = TraceTag;
			//Params.TraceTag = TraceTag;
			Params.bReturnPhysicalMaterial = true;
			FHitResult MuzzleTraceResult{};
			GetWorld()->LineTraceSingleByChannel(
				MuzzleTraceResult,
				GetSocketLocation(MuzzleSocketName),
				EndTrace,
				ECollisionChannel::ECC_Visibility,
				Params
			);
			MuzzleTraceResults.Add(MuzzleTraceResult);
		}

		FVector muzzlePosition = GetSocketLocation(MuzzleSocketName);
		TArray<FVector> tracerPositions;
		TArray<FVector> impactPositions;
		TArray<FVector> impactNormals;
		TArray<int32> impactSurfaceTypes;

		for (int32 i = 0; i < MuzzleTraceResults.Num(); i++)
		{
			FHitResult hitResult = MuzzleTraceResults[i];

			if (hitResult.bBlockingHit)
			{
				tracerPositions.Add(hitResult.Location);
				impactPositions.Add(hitResult.Location);
				impactNormals.Add(hitResult.Normal);
				impactSurfaceTypes.Add(hitResult.PhysMaterial->SurfaceType.GetIntValue());
			}
			else
			{
				tracerPositions.Add(hitResult.TraceEnd);
			}
		}

		if (!IsValid(WeaponFX))
		{
			FTransform spawnTransform{ FRotator(), FVector() };
			auto DeferredWeaponFXActor = Cast<AWeaponFX>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AWeaponFX::StaticClass(), spawnTransform));
			if (DeferredWeaponFXActor != nullptr)
			{
				DeferredWeaponFXActor->WeaponMesh = this;
				DeferredWeaponFXActor->MuzzleFlash_FX = MuzzleFlash_FX;
				DeferredWeaponFXActor->Tracer_FX = Tracer_FX;
				DeferredWeaponFXActor->ShellEject_FX = ShellEject_FX;
				DeferredWeaponFXActor->ShellEjectMesh = ShellEjectMesh;
				DeferredWeaponFXActor->MuzzleSocket = &MuzzleSocketName;
				DeferredWeaponFXActor->ShellEjectSocket = &ShellEjectSocketName;

				UGameplayStatics::FinishSpawningActor(DeferredWeaponFXActor, spawnTransform);
			}

			WeaponFX = DeferredWeaponFXActor;
			WeaponFX->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
		WeaponFX->WeaponFire(tracerPositions);


		if (!IsValid(WeaponDecals))
		{
			FTransform spawnTransform{ FRotator(), FVector() };
			auto DeferredWeaponDecalsActor = Cast<AWeaponDecals>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AWeaponDecals::StaticClass(), spawnTransform));
			if (DeferredWeaponDecalsActor != nullptr)
			{
				DeferredWeaponDecalsActor->ImpactDecals_FX = ImpactDecals_FX;

				UGameplayStatics::FinishSpawningActor(DeferredWeaponDecalsActor, spawnTransform);
			}

			WeaponDecals = DeferredWeaponDecalsActor;
			WeaponDecals->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
		WeaponDecals->WeaponFire(
			impactPositions,
			impactNormals,
			impactSurfaceTypes,
			muzzlePosition
		);

		if (impactPositions.Num() > 0 && !IsValid(WeaponImpacts))
		{
			FTransform spawnTransform{ FRotator(), FVector() };
			auto DeferredWeaponImpactsActor = Cast<AWeaponImpacts>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AWeaponImpacts::StaticClass(), spawnTransform));
			if (DeferredWeaponImpactsActor != nullptr)
			{
				DeferredWeaponImpactsActor->ConcreteImpact_FX = ConcreteImpact_FX;
				DeferredWeaponImpactsActor->GlassImpact_FX = GlassImpact_FX;
				DeferredWeaponImpactsActor->CharacterSparksImpact_FX = CharacterSparksImpact_FX;
				DeferredWeaponImpactsActor->DamageNumber_FX = DamageNumber_FX;
				DeferredWeaponImpactsActor->WeaponRef = this;

				UGameplayStatics::FinishSpawningActor(DeferredWeaponImpactsActor, spawnTransform);
			}

			WeaponImpacts = DeferredWeaponImpactsActor;
			WeaponImpacts->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
		WeaponImpacts->WeaponFire(
			impactPositions,
			impactNormals,
			impactSurfaceTypes,
			muzzlePosition
		);
	}

	WeaponFireAnimateDelegate.ExecuteIfBound();
	
	if (!IsValid(WeaponSounds))
	{
		FTransform spawnTransform{ FRotator(), FVector() };
		auto DeferredWeaponSoundsActor = Cast<AWeaponSounds>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AWeaponSounds::StaticClass(), spawnTransform));
		if (DeferredWeaponSoundsActor != nullptr)
		{
			DeferredWeaponSoundsActor->WeaponRef = this;
			DeferredWeaponSoundsActor->FireSound = FireSound;
			DeferredWeaponSoundsActor->FireSoundInterval = FireDelay * FireSoundDelayScale;

			UGameplayStatics::FinishSpawningActor(DeferredWeaponSoundsActor, spawnTransform);
		}

		WeaponSounds = DeferredWeaponSoundsActor;
		WeaponSounds->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	WeaponSounds->WeaponFire();

	if (IsValid(FireCamShake))
	{
		Character->GetLocalViewingPlayerController()->ClientStartCameraShake(FireCamShake);
	}

	// Try and play a firing animation if specified
	//if (FireAnimation != nullptr)
	//{
	//	// Get the animation object for the arms mesh
	//	UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
	//	if (AnimInstance != nullptr)
	//	{
	//		AnimInstance->Montage_Play(FireAnimation, 1.f);
	//	}
	//}

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
}

void UTP_WeaponComponent::Equip()
{
	if (IsEquipping || Character == nullptr)
	{
		return;
	}
	IsEquipping = true;

	WeaponChangeDelegate.BindUFunction(Cast<UFPAnimInstance>(Character->GetMesh1P()->GetAnimInstance()), FName("SetCurrentWeapon"));
	WeaponChangeDelegate.Execute(this);

	WeaponFireAnimateDelegate.BindUFunction(Cast<UFPAnimInstance>(Character->GetMesh1P()->GetAnimInstance()), FName("Fire"));

	Character->GetWorldTimerManager().SetTimer(EquipDelayTimerHandle, this, &UTP_WeaponComponent::SetIsEquippingFalse, EquipTime, false);
}

void UTP_WeaponComponent::SetIsEquippingFalse()
{
	IsEquipping = false;

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(EquipDelayTimerHandle);
	EquipDelayTimerHandle.Invalidate();
}

void UTP_WeaponComponent::Reload()
{
	if (IsReloading || IsEquipping)
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

			FOnMontageBlendingOutStarted BlendOutDelegate;
			BlendOutDelegate.BindUObject(this, &UTP_WeaponComponent::ReloadAnimationBlendOut);
			Character->GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, ReloadAnimation);
		}
	}
}

void UTP_WeaponComponent::CancelReload()
{
	if (Character != nullptr && ReloadAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Stop(0.25f, ReloadAnimation);

			Stop();
		}
	}
}

void UTP_WeaponComponent::PressedADS()
{
	ADS_Held = true;

	EnterADS();
}

void UTP_WeaponComponent::EnterADS()
{
	if (IsReloading)
	{
		return;
	}

	float newRate = 1.f / ADS_Speed;
	ADSTL->SetPlayRate(newRate);

	Character->ForceStopSprint();
	ADSTL->Play();
}

void UTP_WeaponComponent::ReleasedADS()
{
	ADS_Held = false;
	ADSTL->Reverse();
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
}

void UTP_WeaponComponent::AttachWeapon(AOctahedronCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon yet
	if (Character == nullptr || Character->GetHasWeapon() || Character->GetCurrentWeapon())
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	if (FP_Material != nullptr)
	{
		SetMaterial(0, FP_Material);
	}
	Character->ADS_Offset = ADS_Offset;

	if (ScopeSightMesh != nullptr)
	{
		if (ScopeSightMesh->FP_Material_Holo != nullptr)
		{
			ScopeSightMesh->SetMaterial(0, ScopeSightMesh->FP_Material_Holo);
		}
		if (ScopeSightMesh->FP_Material_Mesh != nullptr)
		{
			ScopeSightMesh->SetMaterial(1, ScopeSightMesh->FP_Material_Mesh);
		}
	}

	// Try and play equip animation if specified
	Equip();

	OnEquipDelegate.Broadcast(Character, this);
	
	// switch bHasWeapon so the animation blueprint can switch to another animation set
	Character->SetHasWeapon(true);
	Character->SetCurrentWeapon(this);

	PCRef = Cast<APlayerController>(Character->GetController());
	// Set up action bindings
	if (PCRef != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PCRef->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PCRef->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::PressedFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::ReleasedFire);

			// Reload
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::PressedReload);

			// Switch Fire Mode
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::PressedSwitchFireMode);

			// ADS
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::PressedADS);
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::ReleasedADS);
		}

		CanFire = true;
	}
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
		GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UTP_WeaponComponent::RecoilTimerFunction, 10.0f, false);

		bRecoil = true;
		bRecoilRecovery = false;
	}
}

//Automatically called in RecoilStart(), no need to call this explicitly
void UTP_WeaponComponent::RecoilTimerFunction()
{
	bRecoil = false;
	GetWorld()->GetTimerManager().PauseTimer(FireTimer);
}

//Called when firing stops
void UTP_WeaponComponent::RecoilStop()
{
	IsShouldRecoil = false;
}

//This function is automatically called, no need to call this. It is inside the Tick function
void UTP_WeaponComponent::RecoveryStart()
{
	if (PCRef->GetControlRotation().Pitch > RecoilStartRot.Pitch)
	{
		bRecoilRecovery = true;
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this, &UTP_WeaponComponent::RecoveryTimerFunction, RecoveryTime, false);
	}
}

//This function too is automatically called from the recovery start function.
void UTP_WeaponComponent::RecoveryTimerFunction()
{
	bRecoilRecovery = false;
}

//Needs to be called on event tick to update the control rotation.
void UTP_WeaponComponent::RecoilTick(float DeltaTime)
{
	// servicable for now, but full auto still have a problem where if you move too far from the origin, the recovery is too strong (above the origin) or there won't be any recovery (below origin)
	float recoiltime;
	FVector RecoilVec;

	if (bRecoil)
	{

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
				GetWorld()->GetTimerManager().ClearTimer(FireTimer);
				bRecoil = false;
				RecoveryStart();
			}
		}
		//UE_LOG(LogTemp, Display, TEXT("deltapitch: %f"), deltaPitch);
	}
	else if (bRecoilRecovery)
	{
		//Recoil resetting
		FRotator tmprot = PCRef->GetControlRotation();
		float deltaPitch = UKismetMathLibrary::NormalizedDeltaRotator(tmprot, RecoilStartRot).Pitch;
		if (deltaPitch > 0)
		{
			FRotator TargetRot = PCRef->GetControlRotation() - RecoilDeltaRot;
			float InterpSpeed = UKismetMathLibrary::MapRangeClamped(deltaPitch, 0.f, MaxRecoilPitch, 3.f, 10.f);
			//UE_LOG(LogTemp, Display, TEXT("interpspeed: %f"), InterpSpeed);
			PCRef->SetControlRotation(UKismetMathLibrary::RInterpTo(PCRef->GetControlRotation(), TargetRot, DeltaTime, InterpSpeed));
			RecoilDeltaRot = RecoilDeltaRot + (PCRef->GetControlRotation() - tmprot);
		}
		else
		{
			bRecoilRecovery = false;
			RecoveryTimer.Invalidate();
		}
		//UE_LOG(LogTemp, Display, TEXT("deltapitch: %f"), deltaPitch);
	}
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (PCRef != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PCRef->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}

void UTP_WeaponComponent::ReloadAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted)
{
	if (bInterrupted)
	{
		bool isTimerActive = GetWorld()->GetTimerManager().IsTimerActive(ReloadDelayTimerHandle);
		if (Character != nullptr && !isTimerActive) // prevent spam reload cancel
		{
			Character->GetWorldTimerManager().SetTimer(ReloadDelayTimerHandle, this, &UTP_WeaponComponent::SetIsReloadingFalse, 0.2f, false);
		}
	}
	else
	{
		SetIsReloadingFalse();
	}
}

void UTP_WeaponComponent::SetIsReloadingFalse()
{
	IsReloading = false;
	Character->GetFPAnimInstance()->IsLeftHandIKActive = true;

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(ReloadDelayTimerHandle);
	ReloadDelayTimerHandle.Invalidate();
}
