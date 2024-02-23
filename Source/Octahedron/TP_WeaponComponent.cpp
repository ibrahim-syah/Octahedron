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

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}


void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	if (IsReloading || IsEquipping)
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

void UTP_WeaponComponent::Stow()
{
}

void UTP_WeaponComponent::Equip()
{
	if (IsEquipping)
	{
		return;
	}
	IsEquipping = true;

	if (Character != nullptr && EquipAnimation != nullptr)
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);

			// Reload
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Reload);
		}
	}
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
