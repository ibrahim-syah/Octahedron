// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "OctahedronCharacter.h"
#include "OctahedronProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Blueprint/UserWidget.h"
#include "SightMeshComponent.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	BoundsScale = 2.f;

	ADSTL = CreateDefaultSubobject<UTimelineComponent>(FName("ADSTL"));
	ADSTL->SetTimelineLength(1.f);
	ADSTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	RecoilTL = CreateDefaultSubobject<UTimelineComponent>(FName("RecoilTL"));
	RecoilTL->SetTimelineLength(3.5f);
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

	if (RecoilPitchCurve != nullptr)
	{
		FOnTimelineFloat onRecoilPitchTLCallback;
		onRecoilPitchTLCallback.BindUFunction(this, FName{ TEXT("RecoilPitchTLCallback") });
		RecoilTL->AddInterpFloat(RecoilPitchCurve, onRecoilPitchTLCallback);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Failed to find recoilpitch curve for this weapon"));
	}

	if (RecoilYawCurve != nullptr)
	{
		FOnTimelineFloat onRecoilYawTLCallback;
		onRecoilYawTLCallback.BindUFunction(this, FName{ TEXT("RecoilYawTLCallback") });
		RecoilTL->AddInterpFloat(RecoilYawCurve, onRecoilYawTLCallback);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Failed to find recoilyaw curve for this weapon"));
	}

	if (ScopeSightMesh != nullptr)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		ScopeSightMesh->AttachToComponent(this, AttachmentRules, FName(TEXT("Sight")));
	}

	MPC_FP_Instance = GetWorld()->GetParameterCollectionInstance(MPC_FP);
}

void UTP_WeaponComponent::PressedFire()
{
	IsPlayerHoldingShootButton = true;
	if (Character == nullptr || Character->GetController() == nullptr || !Character->CanAct() || GetWorld()->GetTimerManager().GetTimerRemaining(FireRateDelayTimerHandle) > 0)
	{
		return;
	}

	const float delay = 60.f / FireRate;

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(FireRateDelayTimerHandle);
	FireRateDelayTimerHandle.Invalidate();
	switch (FireMode)
	{
	case EFireMode::Single:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::Fire, delay, false, 0.f);
		break;
	case EFireMode::Burst:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::BurstFire, delay, true, 0.f);
		break;
	case EFireMode::Auto:
		Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::FullAutoFire, delay, true, 0.f);
		break;
	default:
		break;
	}
}

void UTP_WeaponComponent::ReleasedFire()
{
	IsPlayerHoldingShootButton = false;
	if (Character == nullptr || Character->GetController() == nullptr || !Character->CanAct())
	{
		return;
	}

	StopFire();
}

void UTP_WeaponComponent::PressedReload()
{
	if (Character == nullptr || Character->GetController() == nullptr || !Character->CanAct())
	{
		return;
	}

	Reload();
}

void UTP_WeaponComponent::PressedSwitchFireMode()
{
	if (!CanSwitchFireMode || Character == nullptr || Character->GetController() == nullptr || !Character->CanAct())
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
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
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
		FVector RandomDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(ForwardVector, Spread);
		FVector ResultingVector = RandomDirection * Range;
		FVector EndVector = StartVector + ResultingVector;

		FHitResult CameraTraceResult{};
		auto Params = FCollisionQueryParams();
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

		const FName TraceTag("MyTraceTag");
		GetWorld()->DebugDrawTraceTag = TraceTag;
		Params.TraceTag = TraceTag;
		FHitResult MuzzleTraceResult{};
		GetWorld()->LineTraceSingleByChannel(
			MuzzleTraceResult,
			GetSocketLocation("Muzzle"),
			EndTrace,
			ECollisionChannel::ECC_Visibility,
			Params
		);
	}

	float newRate = 1.f / Recoil_Speed;
	RecoilTL->SetPlayRate(newRate);
	RecoilTL->Play();
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void UTP_WeaponComponent::StopFire()
{
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


	if (EquipAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(EquipAnimation, 1.f);

			FOnMontageBlendingOutStarted BlendOutDelegate;
			BlendOutDelegate.BindUObject(this, &UTP_WeaponComponent::EquipAnimationBlendOut);
			AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, EquipAnimation);
		}
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

	OnEquipDelegate.Broadcast(Character, this);
}

void UTP_WeaponComponent::EquipAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted)
{
	if (bInterrupted)
	{
		bool isTimerActive = GetWorld()->GetTimerManager().IsTimerActive(EquipDelayTimerHandle);
		if (Character != nullptr && !isTimerActive)
		{
			Character->GetWorldTimerManager().SetTimer(EquipDelayTimerHandle, this, &UTP_WeaponComponent::SetIsEquippingFalse, 0.2f, false);
		}
	}
	else
	{
		SetIsEquippingFalse();
	}
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
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(ReloadAnimation, 1.f);

			FOnMontageBlendingOutStarted BlendOutDelegate;
			BlendOutDelegate.BindUObject(this, &UTP_WeaponComponent::ReloadAnimationBlendOut);
			AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, ReloadAnimation);
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

	// Try and play equip animation if specified
	Equip();
	
	// switch bHasWeapon so the animation blueprint can switch to another animation set
	Character->SetHasWeapon(true);
	Character->SetCurrentWeapon(this);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
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

void UTP_WeaponComponent::RecoilPitchTLCallback(float val)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetLocalViewingPlayerController()->AddPitchInput(val);
}

void UTP_WeaponComponent::RecoilYawTLCallback(float val)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetLocalViewingPlayerController()->AddYawInput(val);
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
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

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(ReloadDelayTimerHandle);
	ReloadDelayTimerHandle.Invalidate();
}
