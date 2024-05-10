// Fill out your copyright notice in the Description page of Project Settings.


#include "FPAnimInstance.h"
#include "Octahedron/OctahedronCharacter.h"
#include "Octahedron/TP_WeaponComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"


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
			if (!IsSightTransformSet)
			{
				SetSightTransform();
				SetRelativeHandTransform();
				IsSightTransformSet = true;
			}
			if (!RecoilTransform.Equals(FTransform()) || !FinalRecoilTransform.Equals(FTransform()))
			{
				InterpRecoil(DeltaSeconds);
				InterpFinalRecoil(DeltaSeconds);
			}
			SnapLeftHandToWeapon();
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
		ADS_Alpha_Lerp = FMath::Lerp(0.2f, 1.f, ADS_Alpha);

		ModifyForADS();

		//ADSOffset = Character->GetADSOffset();
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
		IsLeftHandIKActive = true;
		EquipTime = CurrentWeapon->EquipTime;

		RecoilLocMin = CurrentWeapon->RecoilLocMin;
		RecoilLocMax = CurrentWeapon->RecoilLocMin;
		RecoilRotMin = CurrentWeapon->RecoilRotMin;
		RecoilRotMax = CurrentWeapon->RecoilRotMax;

		RecoilLocMinADS = CurrentWeapon->RecoilLocMinADS;
		RecoilLocMaxADS = CurrentWeapon->RecoilLocMinADS;
		RecoilRotMinADS = CurrentWeapon->RecoilRotMinADS;
		RecoilRotMaxADS = CurrentWeapon->RecoilRotMaxADS;
	}
}

void UFPAnimInstance::SetSightTransform()
{
	const FTransform cameraWorldTransform = Character->GetFirstPersonCameraComponent()->GetComponentTransform();
	const FTransform mesh1pWorldTransform = Character->GetMesh1P()->GetComponentTransform();

	const FTransform relativeTransform = UKismetMathLibrary::MakeRelativeTransform(cameraWorldTransform, mesh1pWorldTransform);

	SightTransform.SetLocation(relativeTransform.GetLocation() + relativeTransform.GetRotation().GetForwardVector() * CurrentWeapon->Sight_ForwardLength);
	SightTransform.SetRotation(relativeTransform.Rotator().Quaternion());

}

void UFPAnimInstance::SetRelativeHandTransform()
{
	RelativeHandTransform = UKismetMathLibrary::MakeRelativeTransform(
		CurrentWeapon->GetSocketTransform("Sight"),
		Character->GetMesh1P()->GetSocketTransform("hand_r")
	);
}

void UFPAnimInstance::ModifyForADS()
{
	CamAnimAlpha = CamAnimAlpha * ADS_Alpha_Lerp;
	WalkAnimAlpha = WalkAnimAlpha * ADS_Alpha;
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
	float interpSpeed = (1.f / DeltaSeconds) / 6.f;
	RecoilTransform = UKismetMathLibrary::TInterpTo(RecoilTransform, FinalRecoilTransform, DeltaSeconds, interpSpeed);
}

void UFPAnimInstance::InterpFinalRecoil(float DeltaSeconds)
{
	float interpSpeed = (1.f / DeltaSeconds) / 6.f;
	FinalRecoilTransform = UKismetMathLibrary::TInterpTo(FinalRecoilTransform, FTransform(), DeltaSeconds, interpSpeed);
	
}

void UFPAnimInstance::SnapLeftHandToWeapon()
{

	FTransform TLeftHandSocket = CurrentWeapon->GetSocketTransform(FName("S_LeftHand"));
	FVector boneSpaceLoc;
	FRotator boneSpaceRot;
	Character->GetMesh1P()->TransformToBoneSpace(FName("hand_r"), TLeftHandSocket.GetLocation(), TLeftHandSocket.GetRotation().Rotator(), boneSpaceLoc, boneSpaceRot);
	TLeftHand.SetLocation(boneSpaceLoc);
	TLeftHand.SetRotation(boneSpaceRot.Quaternion());
}

void UFPAnimInstance::Fire()
{
	FVector locMin = FMath::Lerp(RecoilLocMinADS, RecoilLocMin, ADS_Alpha);
	FVector locMax = FMath::Lerp(RecoilLocMaxADS, RecoilLocMax, ADS_Alpha);
	FRotator rotMin = FMath::Lerp(RecoilRotMinADS, RecoilRotMin, ADS_Alpha);
	FRotator rotMax = FMath::Lerp(RecoilRotMaxADS, RecoilRotMax, ADS_Alpha);

	FVector RecoilLoc = FinalRecoilTransform.GetLocation();
	RecoilLoc += FVector(
		FMath::RandRange(locMin.X, locMax.X),
		FMath::RandRange(locMin.Y, locMax.Y),
		FMath::RandRange(locMin.Z, locMax.Z)
	);
	
	FRotator RecoilRot = FinalRecoilTransform.GetRotation().Rotator();
	RecoilRot += FRotator(
		FMath::RandRange(rotMin.Pitch, rotMax.Pitch),
		FMath::RandRange(rotMin.Yaw, rotMax.Yaw),
		FMath::RandRange(rotMin.Roll, rotMax.Roll)
	);

	FinalRecoilTransform.SetLocation(RecoilLoc);
	FinalRecoilTransform.SetRotation(RecoilRot.Quaternion());
}
