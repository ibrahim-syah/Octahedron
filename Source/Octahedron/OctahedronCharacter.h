// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "OctahedronCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UTimelineComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AOctahedronCharacter : public ACharacter
{
	GENERATED_BODY()

	/** FP Root */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* FP_Root;

	/** Mesh_Root spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* Mesh_Root;

	/** Offset Root */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* Offset_Root;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Cam_Root spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* Cam_Root;

	/** Cam Skeleton */
	UPROPERTY(VisibleDefaultsOnly)
	USkeletalMeshComponent* Cam_Skel;

	

	/** First person camera -> instantiate in bp instead! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* CrouchTL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* CrouchAlphaCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseWalkSpeed{ 600.f };

	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void CrouchTLCallback(float val);
	
public:
	AOctahedronCharacter();

protected:
	virtual void BeginPlay();

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasWeapon;

	/** Setter to set the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasWeapon(bool bNewHasWeapon);

	/** Getter for the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasWeapon();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

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
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	FTimerHandle UnCrouchTimerHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float CrouchAlpha;
	float StandHeight{ 96.f };
	float CrouchHeight{ 55.f };



	void CustomCrouch(); // it's called CustomCrouch because Crouch is already provided from ACharacter
	void ReleaseCrouch();
	void OnCheckCanStand();
	void StandUp();



	FTimerHandle CoyoteTimerHandle;
	void CoyoteTimePassed();
	float CoyoteTime{ 0.35f };



	void Dip(float Speed = 1.f, float Strength = 1.f);
	float DipStrength{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* DipTL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* DipAlphaCurve;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void DipTlCallback(float val);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExposedProperties)
	float DipAlpha;
	void LandingDip();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UTimelineComponent* WalkingTL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkLeftRightAlphaCurve;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkLeftRightTLCallback(float val);
	float WalkLeftRightAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkUpDownAlphaCurve;
	UFUNCTION(BlueprintCallable, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	void WalkUpDownTLCallback(float val);
	float WalkUpDownAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Timeline, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* WalkRollAlphaCurve;
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

	
};

