// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponImpacts.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "TP_WeaponComponent.h"
#include "OctahedronCharacter.h"


// Sets default values
AWeaponImpacts::AWeaponImpacts()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWeaponImpacts::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponImpacts::SplitArrayBySurface()
{
	for (int i = 0; i < ImpactSurfaces.Num(); i++)
	{
		switch (ImpactSurfaces[i])
		{
		case 1:
			BodyImpacts.Add(ImpactPositions[i]);
			BodyNormals.Add(ImpactNormals[i]);
			break;
		case 2:
			ConcreteImpacts.Add(ImpactPositions[i]);
			ConcreteNormals.Add(ImpactNormals[i]);
			break;
		case 3:
			GlassImpacts.Add(ImpactPositions[i]);
			GlassNormals.Add(ImpactNormals[i]);
			break;
		default:
			break;
		}
	}
	return;
}

void AWeaponImpacts::WeaponFire(
	TArray<FVector> inImpactPositions,
	TArray<FVector> inImpactNormals,
	TArray<int32> inImpactSurfaces,
	FVector inMuzzlePosition
)
{
	const float delay = 3.f;
	GetWorldTimerManager().SetTimer(CheckDestroyEffectTimerHandle, this, &AWeaponImpacts::CheckDestroyEffect, delay);

	ImpactPositions = inImpactPositions;
	ImpactNormals = inImpactNormals;
	ImpactSurfaces = inImpactSurfaces;
	MuzzlePosition = inMuzzlePosition;

	SplitArrayBySurface();

	for (const FVector &concreteImpact : ConcreteImpacts)
	{
		if (IsValid(ConcreteImpact_FX))
		{
			if (!IsValid(NC_Concrete) || !NC_Concrete->IsActive())
			{
				NC_Concrete = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this,
					ConcreteImpact_FX,
					concreteImpact
				);
			}
			else
			{
				NC_Concrete->SetWorldLocation(concreteImpact);
			}

			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NC_Concrete, FName("ImpactPositions"), ConcreteImpacts);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NC_Concrete, FName("ImpactNormals"), ConcreteNormals);
			NC_Concrete->SetNiagaraVariableInt(FString("NumberOfHits"), ConcreteImpacts.Num());
			NC_Concrete->SetNiagaraVariablePosition(FString("MuzzlePosition"), MuzzlePosition);
			NC_Concrete->ActivateSystem();
		}

		break; // only run once
	}

	for (const FVector &glassImpact : GlassImpacts)
	{
		if (IsValid(GlassImpact_FX))
		{
			if (!IsValid(NC_Glass) || !NC_Glass->IsActive())
			{
				NC_Glass = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this,
					GlassImpact_FX,
					glassImpact
				);
			}
			else
			{
				NC_Glass->SetWorldLocation(glassImpact);
			}

			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NC_Glass, FName("ImpactPositions"), GlassImpacts);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NC_Glass, FName("ImpactNormals"), GlassNormals);
			NC_Glass->SetNiagaraVariableInt(FString("NumberOfHits"), GlassImpacts.Num());
			NC_Glass->SetNiagaraVariablePosition(FString("MuzzlePosition"), MuzzlePosition);
			NC_Glass->ActivateSystem();
		}

		break; // only run once
	}

	if (BodyImpacts.Num() > 0)
	{
		TArray<FVector4> damageInfoArray;
		for (int i = 0; i < BodyImpacts.Num(); i++)
		{
			damageInfoArray.Add(FVector4{
				BodyImpacts[i].X,
				BodyImpacts[i].Y,
				BodyImpacts[i].Z,
				12.f
			});
			if (IsValid(CharacterSparksImpact_FX))
			{
				NC_CharacterSparks = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this,
					CharacterSparksImpact_FX,
					BodyImpacts[i],
					UKismetMathLibrary::MakeRotFromX(BodyNormals[i])
				);
				NC_CharacterSparks->ActivateSystem();
			}
		}
		if (IsValid(DamageNumber_FX) && WeaponRef != nullptr && WeaponRef->GetOwningCharacter()->IsLocallyControlled())
		{
			if (!IsValid(NC_DamageNumber) || !NC_DamageNumber->IsActive())
			{
				NC_DamageNumber = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this,
					DamageNumber_FX,
					BodyImpacts[0],
					UKismetMathLibrary::MakeRotFromX(BodyNormals[0])
				);
			}
			else
			{
				NC_DamageNumber->SetWorldLocation(BodyImpacts[0]);
			}
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NC_DamageNumber, FName("DamageInfo"), damageInfoArray);
			NC_DamageNumber->SetNiagaraVariableBool(FString("Crit"), false);
		}
	}

	BodyImpacts.Empty();
	BodyNormals.Empty();
	GlassImpacts.Empty();
	GlassNormals.Empty();
	ConcreteImpacts.Empty();
	ConcreteNormals.Empty();
}

void AWeaponImpacts::CheckDestroyEffect()
{
	if (IsValid(NC_Concrete) || IsValid(NC_Glass) || IsValid(NC_CharacterSparks) || IsValid(NC_DamageNumber))
	{
		return;
	}
	else {
		Destroy();
	}
}

// Called every frame
//void AWeaponImpacts::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

