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
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TP_WeaponComponent.h"
#include "SightMeshComponent.h"
#include "Public/FPAnimInstance.h"

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
	Mesh_Root->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Mesh_Root->TargetArmLength = 0;
	Mesh_Root->bDoCollisionTest = false;
	Mesh_Root->bUsePawnControlRotation = true;
	Mesh_Root->bInheritPitch = true;
	Mesh_Root->bInheritYaw = true;
	Mesh_Root->bInheritRoll = false;

	Offset_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	Offset_Root->SetupAttachment(Mesh_Root);
	Offset_Root->SetRelativeLocation(FVector(0.f, 0.f, -70.f));

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
	Cam_Root->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Cam_Root->TargetArmLength = 0;
	Cam_Root->bDoCollisionTest = false;
	Cam_Root->bUsePawnControlRotation = true;
	Cam_Root->bInheritPitch = true;
	Cam_Root->bInheritYaw = true;
	Cam_Root->bInheritRoll = false;

	Cam_Skel = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Cam_Skel"));
	Cam_Skel->SetupAttachment(Cam_Root);
	Cam_Skel->SetRelativeLocation(FVector(0.f, 0.f, -60.f));



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

	FOnTimelineFloat onWalkingLeftRightTLCallback;
	onWalkingLeftRightTLCallback.BindUFunction(this, FName{ TEXT("WalkLeftRightTLCallback") });
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
	WalkingTL->AddInterpFloat(WalkLeftRightAlphaCurve, onWalkingLeftRightTLCallback);

	FOnTimelineFloat onWalkingUpDownTLCallback;
	onWalkingUpDownTLCallback.BindUFunction(this, FName{ TEXT("WalkUpDownTLCallback") });
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
	WalkingTL->AddInterpFloat(WalkUpDownAlphaCurve, onWalkingUpDownTLCallback);

	FOnTimelineFloat onWalkingRollTLCallback;
	onWalkingRollTLCallback.BindUFunction(this, FName{ TEXT("WalkRollTLCallback") });
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
	WalkingTL->AddInterpFloat(WalkRollAlphaCurve, onWalkingRollTLCallback);

	FOnTimelineEvent footstepEvent;
	footstepEvent.BindUFunction(this, FName{ TEXT("WalkTLFootstepCallback") });
	WalkingTL->AddEvent(0.35f, footstepEvent);
	WalkingTL->AddEvent(0.85f, footstepEvent);

	FOnTimelineEvent updateWalkEvent;
	updateWalkEvent.BindUFunction(this, FName{ TEXT("WalkTLUpdateEvent") });
	WalkingTL->SetTimelinePostUpdateFunc(updateWalkEvent);

	MoveMode = ECustomMovementMode::Walking;

	SlideTL = CreateDefaultSubobject<UTimelineComponent>(FName("SlideTL"));
	SlideTL->SetTimelineLength(1.f);
	SlideTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onSlideTLCallback;
	onSlideTLCallback.BindUFunction(this, FName{ TEXT("SlideTLCallback") });
	SlideAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("SlideAlphaCurve"));
	KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(0.f, 1.f);
	SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	SlideTL->AddInterpFloat(SlideAlphaCurve, onSlideTLCallback);
	FOnTimelineEvent onSlideTLFinished;
	onSlideTLFinished.BindUFunction(this, FName{ TEXT("FinishedSlideDelegate") });
	SlideTL->SetTimelineFinishedFunc(onSlideTLFinished);
}

void AOctahedronCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (UFPAnimInstance* fpAnimInstance = Cast<UFPAnimInstance>(Mesh1P->GetAnimInstance()))
	{
		FPAnimInstance = fpAnimInstance;
	}

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
		FirstPersonCameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("The camera component is not set!"));
	}

	WalkingTL->Play();
}

void AOctahedronCharacter::Tick(float DeltaTime)
{
	
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
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AOctahedronCharacter::StopMove);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::Look);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AOctahedronCharacter::CustomCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AOctahedronCharacter::ReleaseCrouch);

		// sprint
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AOctahedronCharacter::PressedSprint);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AOctahedronCharacter::UpdateGroundMovementSpeed()
{
	float newWalkSpeed = FMath::Lerp(BaseWalkSpeed, (BaseWalkSpeed * 0.65f), CrouchAlpha);
	GetCharacterMovement()->MaxWalkSpeed = newWalkSpeed;
}

void AOctahedronCharacter::UpdatePlayerCapsuleHeight()
{
	float newCapsuleHalfHeight = FMath::Lerp(StandHeight, CrouchHeight, CrouchAlpha);
	GetCapsuleComponent()->SetCapsuleHalfHeight(newCapsuleHalfHeight, true);
}

void AOctahedronCharacter::CrouchTLCallback(float val)
{
	CrouchAlpha = val;

	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		UpdateGroundMovementSpeed();
		break;
	case ECustomMovementMode::Crouching:
		UpdateGroundMovementSpeed();
		break;
	default:
		break;
	}

	UpdatePlayerCapsuleHeight();
}

void AOctahedronCharacter::CustomCrouch()
{
	CrouchKeyHeld = true;
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		MoveMode = ECustomMovementMode::Crouching;

		// Ensure the timer is cleared by using the timer handle
		GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
		UnCrouchTimerHandle.Invalidate();
		CrouchTL->Play();
		break;
	case ECustomMovementMode::Crouching:
		break;
	case ECustomMovementMode::Sprinting:
		if (GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Falling)
		{
			Sliding();
		}
		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
}

void AOctahedronCharacter::ReleaseCrouch()
{
	CrouchKeyHeld = false;

	CustomUnCrouch();
}

void AOctahedronCharacter::CustomUnCrouch()
{
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		break;
	case ECustomMovementMode::Crouching:

		GetWorldTimerManager().SetTimer(UnCrouchTimerHandle, this, &AOctahedronCharacter::OnCheckCanStand, (1.f / 30.f), true);
		break;
	case ECustomMovementMode::Sprinting:
		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
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
	MoveMode = ECustomMovementMode::Walking;

	// sequence 1
	CrouchTL->Reverse();

	// sequence 2
	if (SprintToggle)
	{
		ForceStartSprint();
	}
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

		CheckStopSprint(MovementVector.Y);
	}
}

void AOctahedronCharacter::StopMove(const FInputActionValue& Value)
{
	CheckStopSprint(0.f);
}

void AOctahedronCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	float LookScaleModifier = 1.f;

	if (CurrentWeapon)
	{
		LookScaleModifier *= FMath::Lerp(1.f, ADSSensitivityScale, CurrentWeapon->ADSAlpha);
	}
	FVector2D LookAxisVector = Value.Get<FVector2D>() * LookScaleModifier;

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

	JumpsLeft = JumpsMax;

	// sequence 1
	// On landing, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();

	// sequence 2
	if (CrouchKeyHeld && MoveMode == ECustomMovementMode::Sprinting)
	{
		ForceStartSlide();
	}
}

void AOctahedronCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	JumpsLeft = FMath::Clamp(JumpsLeft - 1, 0, JumpsMax);
	Dip(5.f, 1.f);

	
	if (float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle); remainingTime > 0.f )
	{
		if (JumpCue != nullptr)
		{
			float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
			UGameplayStatics::PlaySoundAtLocation(this, JumpCue, GetActorLocation());
		}
	}

	// On jump, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
}

bool AOctahedronCharacter::CanJumpInternal_Implementation() const
{
	bool canJump = Super::CanJumpInternal_Implementation();
	float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle);

	bool isTimerActive = GetWorld()->GetTimerManager().IsTimerActive(UnCrouchTimerHandle); // can't jump if there is an obstacle above the player
	bool isSlideTLActive = SlideTL->IsActive();
	bool selected = isSlideTLActive ? SlideTL->GetPlaybackPosition() > 0.25f : true;
	return (canJump || remainingTime > 0.f || JumpsLeft > 0) && (!isTimerActive && selected);
}

void AOctahedronCharacter::CoyoteTimePassed()
{
	JumpsLeft -= 1;
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
	if (LandCue != nullptr)
	{
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		UGameplayStatics::PlaySoundAtLocation(this, LandCue, GetActorLocation(), clampedVelocity);
	}
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
	if (MoveMode != ECustomMovementMode::Sliding && FootstepCue != nullptr)
	{
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		float volumeMultiplier = FMath::Lerp(0.2f, 1.f, normalizedSpeed);
		float pitchMultiplier = FMath::Lerp(0.8f, 1.f, normalizedSpeed);
		UGameplayStatics::PlaySoundAtLocation(this, FootstepCue, GetActorLocation(), volumeMultiplier, pitchMultiplier);
	}

	if (MoveMode == ECustomMovementMode::Sprinting)
	{
		Dip(4.f, 0.35f);
	}
}

void AOctahedronCharacter::WalkTLUpdateEvent()
{
	// update walk anim position
	float lerpedWalkAnimPosX = FMath::Lerp(-0.4f, 0.4f, WalkLeftRightAlpha);
	float lerpedWalkAnimPosZ = FMath::Lerp(-0.35f, 0.2f, WalkUpDownAlpha);
	WalkAnimPos = FVector(lerpedWalkAnimPosX, 0.f, lerpedWalkAnimPosZ);

	// update walk anim rotation
	float lerpedWalkAnimRotPitch = FMath::Lerp(1.f, -1.f, WalkRollAlpha);
	WalkAnimRot = FRotator(lerpedWalkAnimRotPitch, 0.f, 0.f);

	// get alpha of walking intensity
	float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		WalkAnimAlpha = 0.f;
	}
	else
	{
		WalkAnimAlpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f); // had to clamp this because when sprinting, this would jump up beyond 1 and the footstep is too fast
	}

	float lerpedWalkAnimAlpha = FMath::Lerp(0.f, 1.65f, WalkAnimAlpha);
	WalkingTL->SetPlayRate(lerpedWalkAnimAlpha);

	// update location lag vars
	GetVelocityVars();

	// update look input vars
	GetLookInputVars(CamRotCurrent);

	// camera animation
	FVector camOffset;
	float camAnimAlpha;
	ProcCamAnim(camOffset, camAnimAlpha);
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

	interpSpeed = (1.f / deltaTime) / 12.f;
	FRotator targetRInterp = FRotator((LocationLagPos.Z * -2.f), 0.f, 0.f);
	FRotator interpedRot = FMath::RInterpTo(InAirTilt, targetRInterp, deltaTime, interpSpeed);
	InAirTilt = interpedRot;

	FVector targetVInterp = FVector((LocationLagPos.Z * -0.5f), 0.f, 0.f);
	FVector interpedInAirOffsetVec = FMath::VInterpTo(InAirOffset, targetVInterp, deltaTime, interpSpeed);
	InAirOffset = interpedInAirOffsetVec;
}

void AOctahedronCharacter::GetLookInputVars(FRotator CamRotPrev)
{
	// Step 1: determining how much to offset the viewmodel based
	// on our current camera pitch
	FRotator deltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), GetActorRotation());
	float normalizedPitch = UKismetMathLibrary::NormalizeToRange(deltaRotator.Pitch, -90.f, 90.f);
	float lerpedY = FMath::Lerp(3.f, -3.f, normalizedPitch);
	float lerpedZ = FMath::Lerp(2.f, -2.f, normalizedPitch);
	PitchOffsetPos = FVector(0.f, lerpedY, lerpedZ);

	float normalizedFurther = UKismetMathLibrary::NormalizeToRange(normalizedPitch, 0.f, 0.5f);
	float clampedNormalizedPitch = FMath::Clamp(normalizedFurther, 0.f, 1.f);
	float lerpedClampedNormalizedPitch = FMath::Lerp(15.f, 0.f, clampedNormalizedPitch);
	FVector newRelativeLocation = FVector(lerpedClampedNormalizedPitch, FP_Root->GetRelativeLocation().Y, FP_Root->GetRelativeLocation().Z);
	FP_Root->SetRelativeLocation(newRelativeLocation);


	// Step 2: finding the rotation rate of our camera and smoothing
	// the result to use for our weapon sway
	CamRotCurrent = FirstPersonCameraComponent->GetComponentRotation();
	FRotator deltaCamRot = UKismetMathLibrary::NormalizedDeltaRotator(CamRotCurrent, CamRotPrev);
	float deltaCamRotPitch, deltaCamRotYaw, deltaCamRotRoll;
	UKismetMathLibrary::BreakRotator(deltaCamRot, deltaCamRotRoll, deltaCamRotPitch, deltaCamRotYaw);
	float pitchInverse = deltaCamRotPitch * -1.f;
	float clampedPitchInverse = FMath::Clamp(pitchInverse, -5.f, 5.f);
	float clampedYaw = FMath::Clamp(deltaCamRotYaw, -5.f, 5.f);
	FRotator newRotator = FRotator(0.f, clampedYaw, clampedPitchInverse);
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	float weaponWeight = bHasWeapon ? FMath::Clamp(CurrentWeapon->WeaponSwaySpeed, 6.f, 80.f) : 6.f;
	float interpSpeed = (1.f / deltaSeconds) / weaponWeight;
	CamRotRate = UKismetMathLibrary::RInterpTo(CamRotRate, newRotator, deltaSeconds, interpSpeed);


	// Step 3: figuring out the amount to offset our viewmodel by,
	// in order to counteract the rotation of our weapon sway
	float normalizedRoll = UKismetMathLibrary::NormalizeToRange(CamRotRate.Roll, -5.f, 5.f);
	float lerpedRoll = FMath::Lerp(-10.f, 10.f, normalizedRoll);

	float normalizedYaw = UKismetMathLibrary::NormalizeToRange(CamRotRate.Yaw, -5.f, 5.f);
	float lerpedYaw = FMath::Lerp(-6.f, 6.f, normalizedYaw);
	CamRotOffset = FVector(lerpedYaw, 0.f, lerpedRoll);

}

void AOctahedronCharacter::ProcCamAnim(FVector &CamOffsetArg, float &CamAnimAlphaArg)
{
	FTransform spine03Transform = Mesh1P->GetSocketTransform(FName("spine_03"));
	FTransform hand_rTransform = Mesh1P->GetSocketTransform(FName("hand_r"));
	FVector inversedTransformLocation = UKismetMathLibrary::InverseTransformLocation(spine03Transform, hand_rTransform.GetLocation());
	FVector differenceVec = PrevHandLoc - inversedTransformLocation;
	FVector swappedAxesVec = FVector(differenceVec.Y, differenceVec.Z, differenceVec.X);
	CamOffset = swappedAxesVec * FVector(-1.f, 1.f, -1.f);
	FVector multipliedVec = CamOffset * CamStrength;
	PrevHandLoc = inversedTransformLocation;



	UAnimInstance* meshAnimInstance = Mesh1P->GetAnimInstance();
	bool isAnyMontagePlaying = meshAnimInstance->IsAnyMontagePlaying();
	auto currentActiveMontage = meshAnimInstance->GetCurrentActiveMontage();
	bool isMontageActive = meshAnimInstance->Montage_IsActive(currentActiveMontage);
	float lowerSelectFloat = isMontageActive ? 1.f : 0.f;
	float upperSelectFloat = isAnyMontagePlaying ? lowerSelectFloat : 0.f;
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaSeconds) / 24.f;
	FVector interpedVec = UKismetMathLibrary::VInterpTo(CamOffsetCurrent, multipliedVec, deltaSeconds, interpSpeed);
	CamOffsetCurrent = interpedVec.GetClampedToSize(0.f, 10.f);

	interpSpeed = (1.f / deltaSeconds) / 60.f;
	CamAnimAlpha = UKismetMathLibrary::FInterpTo(CamAnimAlpha, upperSelectFloat, deltaSeconds, interpSpeed);

	CamOffsetArg = CamOffsetCurrent;
	CamAnimAlphaArg = CamAnimAlpha;
}

void AOctahedronCharacter::AttachWeapon_Implementation(UTP_WeaponComponent* Weapon)
{
	Weapon->SetOwningWeaponWielder(this);
	if (GetCurrentWeapon() == Weapon)
	{
		return;
	}
	Weapon->SetIsEquippingFalse();
	Weapon->IsEquipping = true;

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	Weapon->AttachToComponent(GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	if (Weapon->FP_Material != nullptr)
	{
		Weapon->SetMaterial(0, Weapon->FP_Material);
	}

	if (Weapon->ScopeSightMesh != nullptr)
	{
		if (Weapon->ScopeSightMesh->FP_Material_Holo != nullptr)
		{
			Weapon->ScopeSightMesh->SetMaterial(0, Weapon->ScopeSightMesh->FP_Material_Holo);
		}
		if (Weapon->ScopeSightMesh->FP_Material_Mesh != nullptr)
		{
			Weapon->ScopeSightMesh->SetMaterial(1, Weapon->ScopeSightMesh->FP_Material_Mesh);
		}
	}

	// ensure current weapon for Character Actor is set to the new one before calling SetCurrentWeapon in anim bp
	SetCurrentWeapon(Weapon);

	// Try and play equip animation if specified
	//GetCurrentWeapon()->WeaponChangeDelegate.AddUnique()
	GetCurrentWeapon()->WeaponChangeDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("SetCurrentWeapon"));
	GetCurrentWeapon()->WeaponChangeDelegate.Execute(GetCurrentWeapon());

	Weapon->Equip();
	if (GetFPAnimInstance())
	{
		GetFPAnimInstance()->Montage_Play(GetCurrentWeapon()->FPEquipAnimation, 1.f);

		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(GetCurrentWeapon(), &UTP_WeaponComponent::EquipAnimationBlendOut);
		GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, GetCurrentWeapon()->FPEquipAnimation);
	}

	Weapon->OnEquipDelegate.Broadcast(this, Weapon);

	// switch bHasWeapon so the animation blueprint can switch to current weapon idle anim
	SetHasWeapon(true);

	APlayerController* PCRef = Cast<APlayerController>(GetController());
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AOctahedronCharacter::PressedFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AOctahedronCharacter::ReleasedFire);

			// Reload
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::PressedReload);

			// Switch Fire Mode
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::PressedSwitchFireMode);

			// ADS
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Triggered, this, &AOctahedronCharacter::PressedADS);
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Completed, this, &AOctahedronCharacter::ReleasedADS);
		}

		Weapon->CanFire = true;
	}
}

void AOctahedronCharacter::DetachWeapon_Implementation()
{
	// Check that the character is valid, and the currently set weapon is this object
	if (!GetHasWeapon() || GetCurrentWeapon()->IsReloading || GetCurrentWeapon()->IsStowing || GetCurrentWeapon()->IsEquipping || GetWorld()->GetTimerManager().GetTimerRemaining(GetCurrentWeapon()->FireRateDelayTimerHandle) > 0)
	{
		return;
	}
	RemoveWeaponInputMapping();
	UTP_WeaponComponent* toBeDetached = GetCurrentWeapon();
	toBeDetached->IsStowing = true;
	//toBeDetached->ExitADS(true);
	toBeDetached->ADSTL->SetNewTime(0.f);
	toBeDetached->ADSTL->Stop();

	// Try and play stow animation
	SetHasWeapon(false);
	SetCurrentWeapon(nullptr);

	toBeDetached->WeaponStowDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("StowCurrentWeapon"));
	toBeDetached->WeaponStowDelegate.Execute();
	toBeDetached->Stow();
}

bool AOctahedronCharacter::InstantDetachWeapon_Implementation()
{
	// Check that the character is valid, and the currently set weapon is this object
	if (!GetHasWeapon() || GetCurrentWeapon()->IsReloading || GetWorld()->GetTimerManager().GetTimerRemaining(GetCurrentWeapon()->FireRateDelayTimerHandle) > 0)
	{
		return false;
	}
	RemoveWeaponInputMapping();
	UTP_WeaponComponent* toBeDetached = GetCurrentWeapon();
	toBeDetached->IsStowing = true;
	toBeDetached->ExitADS(true);

	SetHasWeapon(false);
	SetCurrentWeapon(nullptr);

	toBeDetached->WeaponStowDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("StowCurrentWeapon"));
	toBeDetached->WeaponStowDelegate.Execute();

	// Detach the weapon from the First Person Character
	FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepRelative, true);
	toBeDetached->DetachFromComponent(DetachmentRules);

	toBeDetached->CanFire = false;
	toBeDetached->IsStowing = false;
	toBeDetached->OnStowDelegate.Broadcast(this, toBeDetached);
	return true;
}

void AOctahedronCharacter::OnWeaponFired_Implementation()
{
	// play FP Anim bp Fire() function for weapon recoil kick
	GetFPAnimInstance()->Fire();

	if (IsValid(CurrentWeapon->FireCamShake))
	{
		GetLocalViewingPlayerController()->ClientStartCameraShake(CurrentWeapon->FireCamShake);
	}

	// report noise for AI detection
	MakeNoise(1.f, this, CurrentWeapon->GetComponentLocation());
}

void AOctahedronCharacter::OnWeaponReload_Implementation()
{
	if (CurrentWeapon->FPReloadAnimation != nullptr)
	{
		if (GetFPAnimInstance())
		{
			GetFPAnimInstance()->IsLeftHandIKActive = false;
			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPReloadAnimation, 1.f);
		}
	}
}

void AOctahedronCharacter::OnWeaponStopReloadAnimation_Implementation(float blendTime)
{
	if (CurrentWeapon->FPReloadAnimation != nullptr)
	{
		if (GetFPAnimInstance())
		{
			GetFPAnimInstance()->Montage_Stop(blendTime, CurrentWeapon->FPReloadAnimation);
			CurrentWeapon->Stop();
			CurrentWeapon->SetAnimationMode(CurrentWeapon->DefaultAnimationMode);
		}
	}
}

void AOctahedronCharacter::OnADSTLUpdate_Implementation(float TLValue)
{
	if (!IsValid(CurrentWeapon) || CurrentWeapon->MPC_FP == nullptr)
	{
		return;
	}

	CurrentWeapon->ADSAlpha = TLValue;
	CurrentWeapon->ADSAlphaLerp = FMath::Lerp(0.2f, 1.f, (1.f - CurrentWeapon->ADSAlpha));
	ADSAlpha = CurrentWeapon->ADSAlpha;
	float lerpedFOV = FMath::Lerp(CurrentWeapon->FOV_Base, CurrentWeapon->FOV_ADS, CurrentWeapon->ADSAlpha);
	UCameraComponent* camera = GetFirstPersonCameraComponent();
	camera->SetFieldOfView(lerpedFOV);
	float lerpedIntensity = FMath::Lerp(0.4f, 0.7f, CurrentWeapon->ADSAlpha);
	camera->PostProcessSettings.VignetteIntensity = lerpedIntensity;
	float lerpedFlatFov = FMath::Lerp(90.f, 25.f, CurrentWeapon->ADSAlpha);
	CurrentWeapon->MPC_FP_Instance->SetScalarParameterValue(FName("FOV"), lerpedFlatFov);
	FLinearColor OutColor;
	CurrentWeapon->MPC_FP_Instance->GetVectorParameterValue(FName("Offset"), OutColor);
	float lerpedB = FMath::Lerp(0.f, 30.f, CurrentWeapon->ADSAlpha);
	FLinearColor newColor = FLinearColor(OutColor.R, OutColor.G, lerpedB, OutColor.A);
	CurrentWeapon->MPC_FP_Instance->SetVectorParameterValue(FName("Offset"), newColor);

	float newSpeedMultiplier = FMath::Clamp(CurrentWeapon->ADSAlphaLerp, 0.5f, 1);
	GetCharacterMovement()->MaxWalkSpeed = GetBaseWalkSpeed() * newSpeedMultiplier;
}

void AOctahedronCharacter::RemoveWeaponInputMapping()
{
	APlayerController* PCRef = Cast<APlayerController>(GetController());
	if (PCRef != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PCRef->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PCRef->InputComponent))
		{
			EnhancedInputComponent->ClearActionBindings();
		}
	}
}

void AOctahedronCharacter::PressedFire()
{
	GetCurrentWeapon()->IsWielderHoldingShootButton = true;
	if (GetCurrentWeapon()->IsReloading || GetCurrentWeapon()->IsEquipping || GetCurrentWeapon()->IsStowing || GetWorld()->GetTimerManager().GetTimerRemaining(GetCurrentWeapon()->FireRateDelayTimerHandle) > 0)
	{
		return;
	}
	if (!CanAct())
	{
		GetFPAnimInstance()->SetSprintBlendOutTime(GetFPAnimInstance()->InstantSprintBlendOutTime);
		ForceStopSprint();
	}
	if (GetFPAnimInstance()->Montage_IsPlaying(GetCurrentWeapon()->FPReloadAnimation))
	{
		GetFPAnimInstance()->Montage_Stop(0.f, GetCurrentWeapon()->FPReloadAnimation);
	}
	if (GetCurrentWeapon()->CurrentMagazineCount <= 0)
	{
		GetCurrentWeapon()->StopFire();
		GetCurrentWeapon()->IsWielderHoldingShootButton = false;
		if (IsValid(GetCurrentWeapon()->DryFireSound))
		{
			UGameplayStatics::SpawnSoundAttached(
				GetCurrentWeapon()->DryFireSound,
				GetCurrentWeapon()
			);
		}

		if (IWeaponWielderInterface::Execute_GetRemainingAmmo(this, GetCurrentWeapon()->AmmoType) > 0)
		{
			GetCurrentWeapon()->Reload();
			return;
		}
		return;
	}

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(GetCurrentWeapon()->FireRateDelayTimerHandle);
	GetCurrentWeapon()->FireRateDelayTimerHandle.Invalidate();
	GetCurrentWeapon()->RecoilStart();
	switch (GetCurrentWeapon()->FireMode)
	{
	case EFireMode::Single:
		//Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::SingleFire, FireDelay, false, 0.f);
		GetCurrentWeapon()->SingleFire();
		GetWorldTimerManager().SetTimer(GetCurrentWeapon()->FireRateDelayTimerHandle, GetCurrentWeapon()->FireDelay, false);
		break;
	case EFireMode::Burst:
		GetWorldTimerManager().SetTimer(GetCurrentWeapon()->FireRateDelayTimerHandle, GetCurrentWeapon(), &UTP_WeaponComponent::BurstFire, GetCurrentWeapon()->FireDelay, true, 0.f);
		break;
	case EFireMode::Auto:
		GetWorldTimerManager().SetTimer(GetCurrentWeapon()->FireRateDelayTimerHandle, GetCurrentWeapon(), &UTP_WeaponComponent::FullAutoFire, GetCurrentWeapon()->FireDelay, true, 0.f);
		break;
	default:
		break;
	}
}

void AOctahedronCharacter::ReleasedFire()
{
	GetCurrentWeapon()->IsWielderHoldingShootButton = false;
}

void AOctahedronCharacter::PressedReload()
{
	if (!CanAct())
	{
		ForceStopSprint();
	}

	GetCurrentWeapon()->Reload();
}

void AOctahedronCharacter::PressedSwitchFireMode()
{
	if (!GetCurrentWeapon()->CanSwitchFireMode)
	{
		return;
	}

	GetCurrentWeapon()->SwitchFireMode();
}

void AOctahedronCharacter::PressedADS()
{
	GetCurrentWeapon()->ADS_Held = true;

	EnterADS();
}

void AOctahedronCharacter::EnterADS()
{
	if (!IsValid(GetCurrentWeapon()) || GetCurrentWeapon()->IsReloading || GetCurrentWeapon()->IsStowing || GetCurrentWeapon()->IsEquipping)
	{
		return;
	}
	if (GetFPAnimInstance()->Montage_IsActive(CurrentWeapon->FPReloadAnimation))
	{
		IWeaponWielderInterface::Execute_OnWeaponStopReloadAnimation(this, 0.f);
	}

	GetCurrentWeapon()->ADSTL->SetPlayRate(FMath::Clamp(GetCurrentWeapon()->ADS_Speed, 0.1f, 10.f));

	GetFPAnimInstance()->SetSprintBlendOutTime(0.25f);
	ForceStopSprint();
	GetCurrentWeapon()->ADSTL->Play();
}

void AOctahedronCharacter::ReleasedADS()
{
	GetCurrentWeapon()->ADS_Held = false;
	GetCurrentWeapon()->ADSTL->Reverse();
}

int32 AOctahedronCharacter::GetRemainingAmmo_Implementation(EAmmoType AmmoType)
{
	switch (AmmoType)
	{
	case EAmmoType::Primary:
		return 999;
		break;
	case EAmmoType::Special:
		return SpecialAmmoRemaining;
		break;
	case EAmmoType::Heavy:
		return HeavyAmmoRemaining;
		break;
	default:
		return 0;
	}
}

int32 AOctahedronCharacter::SetRemainingAmmo_Implementation(EAmmoType AmmoType, int32 NewValue)
{
	switch (AmmoType)
	{
	case EAmmoType::Primary:
		return 999;
		break;
	case EAmmoType::Special:
		SpecialAmmoRemaining = NewValue;
		return SpecialAmmoRemaining;
		break;
	case EAmmoType::Heavy:
		HeavyAmmoRemaining = NewValue;
		return HeavyAmmoRemaining;
		break;
	default:
		return 0;
	}
}

void AOctahedronCharacter::PressedSprint()
{
	if (bHasWeapon && CurrentWeapon->GetIsReloading())
	{
		CurrentWeapon->CancelReload(0.25f);
	}

	SprintToggle = !SprintToggle;

	if (SprintToggle)
	{
		if (bHasWeapon)
		{
			CurrentWeapon->ForceStopFire();
		}
		StartSprint();
	}
	else
	{
		StopSprint();
	}
}

void AOctahedronCharacter::ForceStartSprint()
{
	StartSprint();
}

void AOctahedronCharacter::ForceStopSlide()
{
	SlideTL->Stop();
	StopSlide();
}

void AOctahedronCharacter::ForceUnCrouch()
{
	CustomUnCrouch();
}

void AOctahedronCharacter::ForceStartSlide()
{
	Sliding();
}

void AOctahedronCharacter::StartSprint()
{
	if (GetCharacterMovement()->GetLastUpdateVelocity().Length() <= 0.f)
	{
		return;
	}
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		GetFPAnimInstance()->SetSprintBlendOutTime(GetFPAnimInstance()->BaseSprintBlendOutTime);
		MoveMode = ECustomMovementMode::Sprinting;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;

		// sequence 1
		if (ADSAlpha > 0.f && CurrentWeapon != nullptr)
		{
			CurrentWeapon->ExitADS(true);
		}

		// sequence 2
		SprintCharge = 0.f;
		GetWorldTimerManager().SetTimer(SprintTimerHandle, this, &AOctahedronCharacter::SprintChargeIncrease, 0.1f, true);

		break;
	default:
		break;
	}
}

void AOctahedronCharacter::ForceStopSprint()
{
	StopSprint();
}

void AOctahedronCharacter::CheckStopSprint(float InAxis)
{
	if (InAxis < 0.5f)
	{
		StopSprint();
	}
}

void AOctahedronCharacter::StopSprint()
{
	SprintToggle = false;
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();
	SprintCharge = 0.f;
	
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		break;
	case ECustomMovementMode::Crouching:
		break;
	case ECustomMovementMode::Sprinting:
		MoveMode = ECustomMovementMode::Walking;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (CurrentWeapon != nullptr && CurrentWeapon->ADS_Held)
		{
			EnterADS();
		}

		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
}

void AOctahedronCharacter::SprintChargeIncrease()
{
	SprintCharge += 0.1f;

	if (SprintCharge >= 1.f)
	{
		SprintCharge = 1.f;
		GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
		SprintTimerHandle.Invalidate();
	}
}

void AOctahedronCharacter::StartSlide()
{
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();
	SprintToggle = false;
	Dip(0.8f, 0.6f);

	if (SlideCue != nullptr)
	{
		UGameplayStatics::SpawnSoundAttached(SlideCue, RootComponent);
	}

	FVector lastInput = GetCharacterMovement()->GetLastInputVector();
	FVector lastUpdateVelocity = GetCharacterMovement()->GetLastUpdateVelocity();
	float VelocityVectorLength = (lastUpdateVelocity * FVector(1.f, 1.f, 1.f)).Length();

	SlideDirection = lastInput * VelocityVectorLength;
	MoveMode = ECustomMovementMode::Sliding;
	GetController()->SetIgnoreMoveInput(true);

	float crouchPlayRate = FMath::Lerp(1.f, 0.5f, SprintCharge);
	CrouchTL->SetPlayRate(crouchPlayRate);

	float slidePlayRate = FMath::Lerp(2.f, 1.25f, SprintCharge);
	SlideTL->SetPlayRate(slidePlayRate);
	SlideTL->PlayFromStart();
}

void AOctahedronCharacter::Sliding()
{
	StartSlide();
	GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
	UnCrouchTimerHandle.Invalidate();
	CrouchTL->Play();
}

void AOctahedronCharacter::StopSlide()
{
	MoveMode = ECustomMovementMode::Crouching;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * 0.65;
	GetController()->SetIgnoreMoveInput(false);
	CrouchTL->SetPlayRate(1.f);

	SprintCharge = 0.f;
	if (!CrouchKeyHeld)
	{
		ForceUnCrouch();
	}
}

void AOctahedronCharacter::SlideTLCallback(float val)
{
	SlideAlpha = val;
	float A = BaseWalkSpeed * 0.65f;
	float B = BaseWalkSpeed * FMath::Lerp(0.65f, 5.0f, SprintCharge);
	float newMaxSpeed = FMath::Lerp(A, B, SlideAlpha);
	GetCharacterMovement()->MaxWalkSpeed = newMaxSpeed;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f - SlideAlpha;

	AddMovementInput(SlideDirection, SlideAlpha, true);
}

void AOctahedronCharacter::FinishedSlideDelegate()
{
	StopSlide();
}

// can the character perform actions?
// these actions cannot be performed while sprinting
bool AOctahedronCharacter::CanAct()
{
	if (MoveMode != ECustomMovementMode::Sprinting)
	{
		return true;
	}
	return false;
}
