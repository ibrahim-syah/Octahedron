// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ECustomMovementMode.h"
#include "OctahedronCharacter.generated.h"


class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UTimelineComponent;
class TP_WeaponComponent;
struct FInputActionValue;
class UFPAnimInstance;
class UPawnNoiseEmitterComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AOctahedronCharacter : public ACharacter
{
	GENERATED_BODY()

	/** FP Root */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* FP_Root = nullptr;

	/** Mesh_Root spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* Mesh_Root = nullptr;

	/** Offset Root */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* Offset_Root = nullptr;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P = nullptr;

	/** Cam_Root spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* Cam_Root = nullptr;

	/** Cam Skeleton */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Cam_Skel = nullptr;

	

	/** First person camera -> instantiate in bp instead! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent = nullptr;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext = nullptr;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction = nullptr;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void PressedSprint();

	bool SprintToggle;

	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void CheckStopSprint(float InAxis);

	void StopSprint();

	float SprintCharge;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	float SprintSpeedMultiplier{ 1.3f };

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SprintChargeIncrease();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSlide();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void Sliding();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSlide();

	FVector SlideDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* SlideTL = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* SlideAlphaCurve = nullptr;

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void SlideTLCallback(float val);

	float SlideAlpha;

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void FinishedSlideDelegate();

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* CrouchTL = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* CrouchAlphaCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseWalkSpeed{ 600.f };

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void CrouchTLCallback(float val);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio, meta = (AllowPrivateAccess = "true"))
	USoundBase* FootstepCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio, meta = (AllowPrivateAccess = "true"))
	USoundBase* JumpCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio, meta = (AllowPrivateAccess = "true"))
	USoundBase* LandCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio, meta = (AllowPrivateAccess = "true"))
	USoundBase* SlideCue = nullptr;
	
public:
	AOctahedronCharacter();

protected:
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) override;

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction = nullptr;

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasWeapon;

	/** Setter to set the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasWeapon(bool bNewHasWeapon);

	/** Getter for the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasWeapon();

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	UTP_WeaponComponent* CurrentWeapon = nullptr;

	/** Setter to set the bool */
	UFUNCTION(Category = Weapon)
	void SetCurrentWeapon(UTP_WeaponComponent* NewWeapon);

	UFUNCTION(BlueprintPure, Category = Weapon)
	UTP_WeaponComponent* GetCurrentWeapon();

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector ADS_Offset;*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float ADSAlpha;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float ADSSensitivityScale{ 0.3f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	ECustomMovementMode MoveMode;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool CanAct();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceStopSprint();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceStartSprint();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceStopSlide();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceUnCrouch();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ForceStartSlide();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UFPAnimInstance* GetFPAnimInstance() { return FPAnimInstance; }

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	void StopMove(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;
	virtual bool CanJumpInternal_Implementation() const override;


	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	float GetDipAlpha() const { return DipAlpha; }

protected:
	FTimerHandle UnCrouchTimerHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float CrouchAlpha;
	float StandHeight{ 96.f };
	float CrouchHeight{ 55.f };
	bool CrouchKeyHeld;



	void CustomCrouch(); // it's called CustomCrouch because Crouch is already provided from ACharacter
	void ReleaseCrouch();
	void CustomUnCrouch();
	void OnCheckCanStand();
	void UpdateGroundMovementSpeed();
	void UpdatePlayerCapsuleHeight();
	void StandUp();



	FTimerHandle CoyoteTimerHandle;
	void CoyoteTimePassed();
	float CoyoteTime{ 0.35f };



	void Dip(float Speed = 1.f, float Strength = 1.f);
	float DipStrength{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* DipTL = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* DipAlphaCurve = nullptr;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void DipTlCallback(float val);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float DipAlpha;
	void LandingDip();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* WalkingTL = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkLeftRightAlphaCurve = nullptr;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkLeftRightTLCallback(float val);
	float WalkLeftRightAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkUpDownAlphaCurve = nullptr;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkUpDownTLCallback(float val);
	float WalkUpDownAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkRollAlphaCurve = nullptr;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkRollTLCallback(float val);
	float WalkRollAlpha;

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkTLFootstepCallback();

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkTLUpdateEvent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector WalkAnimPos;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FRotator WalkAnimRot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float WalkAnimAlpha;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector LocationLagPos;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void GetVelocityVars();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector PitchOffsetPos;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector CamRotOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FRotator CamRotCurrent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FRotator CamRotRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector InAirOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FRotator InAirTilt;
	UFUNCTION(BlueprintCallable, Category = WeaponSway, meta = (AllowPrivateAccess = "true"))
	void GetLookInputVars(FRotator CamRotPrev);


	UFUNCTION(BlueprintCallable, Category = Camera, meta = (AllowPrivateAccess = "true"))
	void ProcCamAnim(FVector &CamOffsetArg, float &CamAnimAlphaArg);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector PrevHandLoc;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector CamOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ExposedProperties)
	float CamStrength{ 25.f };
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	FVector CamOffsetCurrent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float CamAnimAlpha{ 0.f };

	FTimerHandle SprintTimerHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float SprintAlpha;

private:

	int32 JumpsLeft{ 2 };
	int32 JumpsMax{ 2 };
	UFPAnimInstance* FPAnimInstance = nullptr;

public:
	FVector GetLocationLagPos() { return LocationLagPos; }
	float GetCrouchAlpha() { return CrouchAlpha; }
	FVector GetWalkAnimPos() { return WalkAnimPos; }
	FRotator GetWalkAnimRot() { return WalkAnimRot; }
	float GetWalkAnimAlpha() { return WalkAnimAlpha; }
	float GetDipAlpha() { return DipAlpha; }
	FVector GetPitchOffsetPos() { return PitchOffsetPos; }
	FVector GetCamRotOffset() { return CamRotOffset; }
	FRotator GetCamRotCurrent() { return CamRotCurrent; }
	FRotator GetCamRotRate() { return CamRotRate; }
	FRotator GetInAirTilt() { return InAirTilt; }
	FVector GetInAirOffset() { return InAirOffset; }
	FVector GetCamOffsetCurrent() { return CamOffsetCurrent; }
	float GetCamAnimAlpha() { return CamAnimAlpha; }
	float GetADSAlpha() { return ADSAlpha; }
	//FVector GetADSOffset() { return ADS_Offset; }
	ECustomMovementMode GetMoveMode() { return MoveMode; }
};

