// Copyright Epic Games, Inc. All Rights Reserved.

#include "OctahedronCharacter.h"
#include "OctahedronProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AOctahedronCharacter

AOctahedronCharacter::AOctahedronCharacter()
{
	// Character doesnt have a rifle at start
	bHasWeapon = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 96.0f);

	// default character movement values for responsive and weighty control
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 3072.f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f;
	GetCharacterMovement()->PerchRadiusThreshold = 30.f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->JumpZVelocity = 750.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 200.f;
	GetCharacterMovement()->AirControl = 0.275f;

	FP_Root = CreateDefaultSubobject<USceneComponent>(TEXT("FP_Root"));
	FP_Root->SetupAttachment(GetCapsuleComponent());

	Mesh_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Mesh_Root"));
	Mesh_Root->SetupAttachment(FP_Root);
	Mesh_Root->TargetArmLength = 0;
	Mesh_Root->bDoCollisionTest = false;
	Mesh_Root->bUsePawnControlRotation = true;
	Mesh_Root->bInheritPitch = true;
	Mesh_Root->bInheritYaw = true;
	Mesh_Root->bInheritRoll = false;

	Offset_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	Offset_Root->SetupAttachment(Mesh_Root);
	Offset_Root->SetRelativeLocation(FVector(0.f, 0.f, -10.f));

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(Offset_Root);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh1P->SetRelativeLocation(FVector(0.f, 0.f, -96.f));

	Cam_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Cam_Root"));
	Cam_Root->SetupAttachment(FP_Root);
	Cam_Root->TargetArmLength = 0;
	Cam_Root->bDoCollisionTest = false;
	Cam_Root->bUsePawnControlRotation = true;
	Cam_Root->bInheritPitch = true;
	Cam_Root->bInheritYaw = true;
	Cam_Root->bInheritRoll = false;

	Cam_Skel = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Cam_Skel"));
	Cam_Skel->SetupAttachment(Cam_Root);



	CrouchTL = CreateDefaultSubobject<UTimelineComponent>(FName("CrouchTL"));
	CrouchTL->SetTimelineLength(0.2f);
	CrouchTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onCrouchTLCallback;
	onCrouchTLCallback.BindUFunction(this, FName{ TEXT("CrouchTLCallback") });
	CrouchAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("CrouchAlphaCurve"));
	FKeyHandle KeyHandle = CrouchAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = CrouchAlphaCurve->FloatCurve.AddKey(0.2f, 1.f);
	CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	CrouchTL->AddInterpFloat(CrouchAlphaCurve, onCrouchTLCallback);

	

	DipTL = CreateDefaultSubobject<UTimelineComponent>(FName("DipTL"));
	DipTL->SetTimelineLength(1.f);
	DipTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onDipTLCallback;
	onDipTLCallback.BindUFunction(this, FName{ TEXT("DipTLCallback") });
	DipAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("DipAlphaCurve"));
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.2f, 0.95f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.63f, 0.12f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	DipTL->AddInterpFloat(DipAlphaCurve, onDipTLCallback);

	WalkingTL = CreateDefaultSubobject<UTimelineComponent>(FName("WalkingTL"));
	WalkingTL->SetTimelineLength(1.f);
	WalkingTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
	WalkingTL->SetLooping(true);

	FOnTimelineFloat onWalkingTLCallback;

	onWalkingTLCallback.BindUFunction(this, FName{ TEXT("WalkLeftRightTLCallback") });
	WalkLeftRightAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkLeftRightAlphaCurve"));
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.25f, 0.5f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.5f, 1.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.75f, 0.5f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkLeftRightAlphaCurve, onWalkingTLCallback);

	onWalkingTLCallback.BindUFunction(this, FName{ TEXT("WalkUpDownTLCallback") });
	WalkUpDownAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkUpDownAlphaCurve"));
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.3f, 1.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.5f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.8f, 1.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkUpDownAlphaCurve, onWalkingTLCallback);

	onWalkingTLCallback.BindUFunction(this, FName{ TEXT("WalkRollTLCallback") });
	WalkRollAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkRollAlphaCurve"));
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.f, 0.18f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.15f, 0.f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.4f, 0.5f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.65f, 1.f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.9f, 0.5f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(1.f, 0.18f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkRollAlphaCurve, onWalkingTLCallback);

	FOnTimelineEvent footstepEvent;
	footstepEvent.BindUFunction(this, FName{ TEXT("WalkTLFootstepCallback") });
	WalkingTL->AddEvent(0.35f, footstepEvent);
	WalkingTL->AddEvent(0.85f, footstepEvent);

	FOnTimelineEvent updateWalkEvent;
	updateWalkEvent.BindUFunction(this, FName{ TEXT("WalkTLUpdateEvent") });
	WalkingTL->SetTimelinePostUpdateFunc(updateWalkEvent);
}

void AOctahedronCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Ensure camera is set in blueprint class
	if (auto camera = Cast<UCameraComponent>(Cam_Skel->GetChildComponent(0)))
	{
		FirstPersonCameraComponent = camera;
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("The camera component is not set!"));
	}

	WalkingTL->Play();
}

//////////////////////////////////////////////////////////////////////////// Input

void AOctahedronCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::Look);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AOctahedronCharacter::CustomCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AOctahedronCharacter::ReleaseCrouch);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AOctahedronCharacter::CrouchTLCallback(float val)
{
	CrouchAlpha = val;
	// Update ground movement speed
	float newWalkSpeed = FMath::Lerp(BaseWalkSpeed, (BaseWalkSpeed * .5f), CrouchAlpha);
	GetCharacterMovement()->MaxWalkSpeed = newWalkSpeed;

	// Update capsule half height
	float newCapsuleHalfHeight = FMath::Lerp(StandHeight, CrouchHeight, CrouchAlpha);
	GetCapsuleComponent()->SetCapsuleHalfHeight(newCapsuleHalfHeight);
}

void AOctahedronCharacter::CustomCrouch()
{
	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
	UnCrouchTimerHandle.Invalidate();
	CrouchTL->Play();
}

void AOctahedronCharacter::ReleaseCrouch()
{
	GetWorldTimerManager().SetTimer(UnCrouchTimerHandle, this, &AOctahedronCharacter::OnCheckCanStand, (1.f/30.f), true);
}

void AOctahedronCharacter::OnCheckCanStand()
{

	FVector sphereTraceLocation = FVector(GetActorLocation().X, GetActorLocation().Y, (GetActorLocation().Z + CrouchHeight));

	FVector SphereStart = FVector(GetActorLocation().X, GetActorLocation().Y, (GetActorLocation().Z + CrouchHeight));

	float lerpedHeight = FMath::Lerp(0.f, (StandHeight - CrouchHeight), CrouchAlpha);
	float scaledLerpedHeight = lerpedHeight * 1.1f;
	float sphereEndZ = (GetActorLocation().Z + CrouchHeight) + scaledLerpedHeight;
	FVector SphereEnd = FVector(GetActorLocation().X, GetActorLocation().Y, sphereEndZ);
	float sphereRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5f;
	FCollisionShape Sphere{ FCollisionShape::MakeSphere(sphereRadius)};
	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(this);
	FHitResult HitResult;

	bool isStuck = GetWorld()->SweepSingleByChannel(HitResult, SphereStart, SphereEnd, FQuat::Identity, ECollisionChannel::ECC_Visibility, Sphere, Params);
	bool isFalling = GetCharacterMovement()->IsFalling();

	if (!isStuck || isFalling)
	{
		StandUp();
		GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
		UnCrouchTimerHandle.Invalidate();
	}



}

void AOctahedronCharacter::StandUp()
{
	CrouchTL->Reverse();
}


void AOctahedronCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AOctahedronCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AOctahedronCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		// change coyote time based on speed
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		float alpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f);
		float lerpedValue = FMath::Lerp(0.25f, 1.f, alpha);
		float time = CoyoteTime * lerpedValue;
		GetWorldTimerManager().SetTimer(CoyoteTimerHandle, this, &AOctahedronCharacter::CoyoteTimePassed, time, true);
	}
}

void AOctahedronCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	LandingDip();

	// On landing, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
}

void AOctahedronCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	Dip(5.f, 1.f);

	// On jump, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
}

bool AOctahedronCharacter::CanJumpInternal_Implementation() const
{
	bool canJump = Super::CanJumpInternal_Implementation();
	float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle);

	bool isTimerActive = GetWorld()->GetTimerManager().IsTimerActive(UnCrouchTimerHandle); // can't jump if there is an obstacle above the player
	return (remainingTime > 0.f || canJump) && !isTimerActive;
}

void AOctahedronCharacter::CoyoteTimePassed()
{

}

void AOctahedronCharacter::Dip(float Speed, float Strength)
{
	// set dip param
	DipTL->SetPlayRate(Speed);
	DipStrength = Strength;
	DipTL->PlayFromStart();
}

void AOctahedronCharacter::DipTlCallback(float val)
{
	// update dip alpha
	DipAlpha = val * DipStrength;

	// update fp_root
	float lerpedZValue = FMath::Lerp(0.f, -10.f, DipAlpha);
	FVector newLocation = FVector(FP_Root->GetRelativeLocation().X, FP_Root->GetRelativeLocation().Y, lerpedZValue);
	FP_Root->SetRelativeLocation(newLocation);
}

void AOctahedronCharacter::LandingDip()
{
	float lastZVelocity = GetCharacterMovement()->GetLastUpdateVelocity().Z;
	float ZVectorLength = FVector(0.f, 0.f, lastZVelocity).Length();
	float jumpZvelocity = GetCharacterMovement()->JumpZVelocity;
	float normalizedVelocity = UKismetMathLibrary::NormalizeToRange(ZVectorLength, 0.f, jumpZvelocity);
	float clampedVelocity = FMath::Clamp(normalizedVelocity, 0.f, 1.f);
	Dip(3.f, clampedVelocity);
}

void AOctahedronCharacter::WalkLeftRightTLCallback(float val)
{
	WalkLeftRightAlpha = val;
}

void AOctahedronCharacter::WalkUpDownTLCallback(float val)
{
	WalkUpDownAlpha = val;
}

void AOctahedronCharacter::WalkRollTLCallback(float val)
{
	WalkRollAlpha = val;
}

void AOctahedronCharacter::WalkTLFootstepCallback()
{
}

void AOctahedronCharacter::WalkTLUpdateEvent()
{
	// update walk anim position
	float lerpedWalkAnimPosX = FMath::Lerp(-0.4f, 0.4f, WalkLeftRightAlpha);
	float lerpedWalkAnimPosZ = FMath::Lerp(-0.35f, 0.2f, WalkUpDownAlpha);
	WalkAnimPos = FVector(lerpedWalkAnimPosX, 0.f, lerpedWalkAnimPosZ);

	// update walk anim rotation
	float lerpedWalkAnimRotPitch = FMath::Lerp(1.f, -1.f, WalkRollAlpha);
	WalkAnimRot = FRotator(0.f, lerpedWalkAnimRotPitch, 0.f);

	// get alpha of walking intensity
	float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		WalkAnimAlpha = 0.f;
	}
	else
	{
		WalkAnimAlpha = normalizedSpeed;
	}

	float lerpedWalkAnimAlpha = FMath::Lerp(0.f, 1.65f, WalkAnimAlpha);
	WalkingTL->SetPlayRate(lerpedWalkAnimAlpha);

	// update location lag vars
	GetVelocityVars();
}

void AOctahedronCharacter::GetVelocityVars()
{
	float velocityDotForwardVec = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
	float velocityDotRightVec = FVector::DotProduct(GetVelocity(), GetActorRightVector());
	float velocityDotUpVec = FVector::DotProduct(GetVelocity(), GetActorUpVector());

	float Y = velocityDotForwardVec / (BaseWalkSpeed * -1.f);
	float X = velocityDotRightVec / BaseWalkSpeed;
	float Z = velocityDotUpVec / GetCharacterMovement()->JumpZVelocity * -1.f;

	FVector resultingVec = FVector(X, Y, Z);
	FVector scaledVec = resultingVec * 2.f;
	FVector ClampedVectorSize = scaledVec.GetClampedToSize(0.f, 4.f);
	
	float deltaTime = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaTime) / 6.f;
	FVector interpedVec = FMath::VInterpTo(LocationLagPos, ClampedVectorSize, deltaTime, interpSpeed);
	LocationLagPos = interpedVec;
}


void AOctahedronCharacter::SetHasWeapon(bool bNewHasRifle)
{
	bHasWeapon = bNewHasRifle;
}

bool AOctahedronCharacter::GetHasWeapon()
{
	return bHasWeapon;
}