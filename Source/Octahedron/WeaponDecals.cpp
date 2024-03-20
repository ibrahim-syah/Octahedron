// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponDecals.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

// Sets default values
AWeaponDecals::AWeaponDecals()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWeaponDecals::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponDecals::RemoveInvalidSurfaces()
{
	TArray<int32> localImpactSurfaces = ImpactSurfaces;
	for (int i = localImpactSurfaces.Num() - 1; i >= 0; i--)
	{
		int32 surface = localImpactSurfaces[i];
		if (surface == 0 || surface == 1) // type 1 is body
		{
			ImpactSurfaces.Pop();
			ImpactPositions.Pop();
			ImpactNormals.Pop();
		}
	}
}

void AWeaponDecals::WeaponFire(TArray<FVector> InImpactPositions, TArray<FVector> InImpactNormals, TArray<int32> InImpactSurfaces, FVector InMuzzlePosition)
{
	ImpactPositions = InImpactPositions;
	ImpactNormals = InImpactNormals;
	ImpactSurfaces = InImpactSurfaces;
	MuzzlePosition = InMuzzlePosition;

	RemoveInvalidSurfaces();

	// Impact Decal FX
	if (IsValid(ImpactDecals_FX))
	{
		if (!IsValid(NC_ImpactDecals) || !NC_ImpactDecals->IsActive())
		{
			NC_ImpactDecals = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this,
				ImpactDecals_FX,
				FVector(0.f, 0.f, 0.f));

			ImpactDecalsTrigger = false;
		}

		ImpactDecalsTrigger = !ImpactDecalsTrigger;

		/*for (auto impactSurface : ImpactSurfaces)
		{
			int32 byte = (int32)impactSurface;
			ImpactSurfacesInt.Add(byte);
		}*/
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NC_ImpactDecals, FName("ImpactSurfaces"), ImpactSurfaces);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NC_ImpactDecals, FName("ImpactPositions"), ImpactPositions);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NC_ImpactDecals, FName("ImpactNormals"), ImpactNormals);
		NC_ImpactDecals->SetNiagaraVariableInt(FString("NumberOfHits"), ImpactPositions.Num());
		NC_ImpactDecals->SetNiagaraVariablePosition(FString("MuzzlePosition"), MuzzlePosition);
		NC_ImpactDecals->SetNiagaraVariableBool(FString("Trigger"), ImpactDecalsTrigger);
	}

	const float delay = 3.f;
	GetWorldTimerManager().SetTimer(CheckDestroyEffectTimerHandle, this, &AWeaponDecals::CheckDestroyEffect, delay, true, delay);
}

void AWeaponDecals::CheckDestroyEffect()
{
	if (IsValid(NC_ImpactDecals))
	{
		return;
	}
	else {
		Destroy();
	}
}

// Called every frame
//void AWeaponDecals::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

