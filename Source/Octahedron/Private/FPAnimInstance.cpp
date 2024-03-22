// Fill out your copyright notice in the Description page of Project Settings.


#include "FPAnimInstance.h"
#include "Octahedron/OctahedronCharacter.h"
#include "Octahedron/TP_WeaponComponent.h"
#include "Kismet/KismetMathLibrary.h"

UFPAnimInstance::UFPAnimInstance()
{
}



void UFPAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	APawn* pawn = TryGetPawnOwner();
	if (IsValid(pawn))
	{
		Character = Cast<AOctahedronCharacter>(pawn);
	}
}

void UFPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (IsValid(Character))
	{
		if (IsValid(CurrentWeapon) && IsValid(CurrentWeaponIdlePose))
		{
			if (!RecoilTransform.Equals(FTransform()) || !FinalRecoilTransform.Equals(FTransform()))
			{
				InterpRecoil(DeltaSeconds);
				InterpFinalRecoil(DeltaSeconds);
			}

			FTransform A = CurrentWeapon->GetSocketTransform(FName("S_LeftHand"));
			FTransform rightHandTransform = Character->GetMesh1P()->GetSocketTransform(FName("hand_r"));
			LeftHandIK = UKismetMathLibrary::MakeRelativeTransform(A, rightHandTransform);
		}

		LocationLagPos = Character->GetLocationLagPos();
		CrouchAlpha = Character->GetCrouchAlpha();
		WalkAnimPos = Character->GetWalkAnimPos();
		WalkAnimRot = Character->GetWalkAnimRot();
		WalkAnimAlpha = Character->GetWalkAnimAlpha();
		DipAlpha = Character->GetDipAlpha();
		PitchOffsetPos = Character->GetPitchOffsetPos();
		CamRotOffset = Character->GetCamRotOffset();
		CamRotCurrent = Character->GetCamRotCurrent();
		CamRotRate = Character->GetCamRotRate();
		InAirTilt = Character->GetInAirTilt();
		InAirOffset = Character->GetInAirOffset();

		FVector characterOffsetCurrent = Character->GetCamOffsetCurrent();
		FVector reverseXY = FVector(characterOffsetCurrent.Y, characterOffsetCurrent.X, characterOffsetCurrent.Z);
		FVector inverseX = reverseXY * FVector(-1.f, 1.f, 1.f);
		CamOffset = inverseX;

		CamAnimAlpha = Character->GetCamAnimAlpha();

		float interpSpeed = (1.f / DeltaSeconds) / 10.f;
		CamOffsetCurrent = UKismetMathLibrary::VInterpTo(CamOffsetCurrent, CamOffset, DeltaSeconds, interpSpeed);

		ADS_Alpha = (1.f - Character->GetADSAlpha());
		ADS_Alpha_Lerp = FMath::Lerp(0.3f, 1.f, ADS_Alpha);

		ModifyForADS();

		ADSOffset = Character->GetADSOffset();
		MoveMode = Character->GetMoveMode();

		ModifyForSprint(DeltaSeconds);
	}
}

void UFPAnimInstance::SetCurrentWeapon(UTP_WeaponComponent* Weapon)
{
	if (IsValid(Weapon))
	{
		IsHasWeapon = true;
		CurrentWeapon = Weapon;
		CurrentWeaponIdlePose = CurrentWeapon->IdlePose;
	}
}

void UFPAnimInstance::ModifyForADS()
{
	CamAnimAlpha = CamAnimAlpha * ADS_Alpha_Lerp;
	WalkAnimAlpha = WalkAnimAlpha * ADS_Alpha_Lerp;
	CrouchAlpha = CrouchAlpha * ADS_Alpha;
	DipAlpha = DipAlpha * ADS_Alpha_Lerp;
}

void UFPAnimInstance::ModifyForSprint(float DeltaSeconds)
{
	FVector XZZ = FVector(WalkAnimPos.X, WalkAnimPos.Z, WalkAnimPos.Z);
	SprintAnimPos = XZZ * FVector(2.f, 5.f, 1.f);

	float interpSpeed = (1.f / DeltaSeconds) / 12.f;
	FRotator scaledWalkAnimRot = WalkAnimRot * -5.f;
	SprintAnimRot = UKismetMathLibrary::RInterpTo(SprintAnimRot, scaledWalkAnimRot, DeltaSeconds, interpSpeed);
}

void UFPAnimInstance::InterpRecoil(float DeltaSeconds)
{
	float interpSpeed = (1.f / DeltaSeconds) / 10.f;
	RecoilTransform = UKismetMathLibrary::TInterpTo(RecoilTransform, FinalRecoilTransform, DeltaSeconds, interpSpeed);
}

void UFPAnimInstance::InterpFinalRecoil(float DeltaSeconds)
{
	float interpSpeed = (1.f / DeltaSeconds) / 10.f;
	FinalRecoilTransform = UKismetMathLibrary::TInterpTo(FinalRecoilTransform, FTransform(), DeltaSeconds, interpSpeed);
	
}

void UFPAnimInstance::Fire()
{
	FVector RecoilLoc = FinalRecoilTransform.GetLocation();
	RecoilLoc += FVector(
		FMath::RandRange(-0.1f, 0.1f) * ADS_Alpha_Lerp,
		FMath::RandRange(-3.f, -1.f) * ADS_Alpha_Lerp,
		FMath::RandRange(0.2f, 1.f) * ADS_Alpha_Lerp
	);
	
	FRotator RecoilRot = FinalRecoilTransform.GetRotation().Rotator();
	RecoilRot += FRotator(
		FMath::RandRange(-5.f, 5.f) * ADS_Alpha_Lerp,
		FMath::RandRange(-1.f, 1.f) * ADS_Alpha_Lerp,
		FMath::RandRange(-3.f, -1.f) * ADS_Alpha_Lerp
	);

	FinalRecoilTransform.SetLocation(RecoilLoc);
	FinalRecoilTransform.SetRotation(RecoilRot.Quaternion());
}
