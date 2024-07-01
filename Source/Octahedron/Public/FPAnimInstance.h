// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Octahedron/ECustomMovementMode.h"
#include "FPAnimInstance.generated.h"

class AOctahedronCharacter;
class UTP_WeaponComponent;

/**
 * 
 */
UCLASS()
class OCTAHEDRON_API UFPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UFPAnimInstance();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeBeginPlay() override;

	UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	void SetCurrentWeapon(UTP_WeaponComponent* Weapon);

	UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	void StowCurrentWeapon();

	UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	void Fire();

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	bool IsLeftHandIKActive;

	float BaseSprintBlendOutTime = 0.45f;
	float InstantSprintBlendOutTime = 0.f;
	void SetSprintBlendOutTime(float blendOutTime) { SprintBlendOutTime = blendOutTime; };

protected:
	bool IsRecoilKicking = false;
	float RecoilKickInterpSpeed = 6.f;
	bool IsRecoilRecovering = false;
	float RecoilRecoveryInterpSpeed = 6.f;
	void SetSightTransform();
	void SetRelativeHandTransform();
	void ModifyForADS();
	void ModifyForSprint(float DeltaSeconds);
	//void InterpRecoil(float DeltaSeconds);
	//void InterpFinalRecoil(float DeltaSeconds);
	void InterpRecoilKick(float DeltaSeconds);
	void InterpRecoilRecovery(float DeltaSeconds);
	void SnapLeftHandToWeapon();

protected:
	bool IsMoving;
	bool IsInAir;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	bool IsHasWeapon;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float EquipTime{ 1.f };

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	AOctahedronCharacter* Character = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	UTP_WeaponComponent* CurrentWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector LocationLagPos;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float CrouchAlpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector WalkAnimPos;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FRotator WalkAnimRot;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float WalkAnimAlpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float DipAlpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector PitchOffsetPos;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector CamRotOffset;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FRotator CamRotCurrent;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FRotator CamRotRate;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FRotator InAirTilt;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector InAirOffset;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector CamOffset;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float CamAnimAlpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector CamOffsetCurrent;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform SightTransform;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform RelativeHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float IsADS;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float IsADSAlpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float ADS_Alpha;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float ADS_Alpha_Lerp;

	/*UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector ADSOffset;*/

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	UAnimSequence* CurrentWeaponIdlePose = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	ECustomMovementMode MoveMode;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FVector SprintAnimPos;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FRotator SprintAnimRot;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	float SprintBlendOutTime = 0.45f;

	/*UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform LeftHandIK;*/

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform TLeftHand;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform RecoilTransform;

	UPROPERTY(BlueprintReadOnly, Category = ExposedProperties)
	FTransform FinalRecoilTransform;

	FVector RecoilLocMin{ -0.1f, -3.f, 0.2f };
	FVector RecoilLocMax{ 0.1f, -1.f, 1.f };
	FRotator RecoilRotMin{ -5.f, -1.f, -3.f };
	FRotator RecoilRotMax{ 5.f, 1.f, -1.f };

	FVector RecoilLocMinADS{ 0.f, -7.f, 0.f };
	FVector RecoilLocMaxADS{ 0.f, -6.f, 0.f };
	FRotator RecoilRotMinADS{ 0.f, 0.f, 0.f };
	FRotator RecoilRotMaxADS{ 0.f, 0.f, 0.f };

};
