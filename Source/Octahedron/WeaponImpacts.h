// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponImpacts.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UTP_WeaponComponent;

UCLASS()
class OCTAHEDRON_API AWeaponImpacts : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponImpacts();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpactNormals;
	TArray<int32> ImpactSurfaces;
	FVector MuzzlePosition;

	TArray<FVector> BodyImpacts;
	TArray<FVector> BodyNormals;
	TArray<FVector> ConcreteImpacts;
	TArray<FVector> ConcreteNormals;
	TArray<FVector> GlassImpacts;
	TArray<FVector> GlassNormals;

	void SplitArrayBySurface();

	UNiagaraComponent* NC_Concrete;
	UNiagaraComponent* NC_Glass;
	UNiagaraComponent* NC_CharacterSparks;
	UNiagaraComponent* NC_DamageNumber;

	FTimerHandle CheckDestroyEffectTimerHandle;
	void CheckDestroyEffect();

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	void WeaponFire(
		TArray<FVector> inImpactPositions,
		TArray<FVector> inImpactNormals,
		TArray<int32> inImpactSurfaces,
		FVector inMuzzlePosition
	);

	UNiagaraSystem* ConcreteImpact_FX;
	UNiagaraSystem* GlassImpact_FX;
	UNiagaraSystem* CharacterSparksImpact_FX;
	UNiagaraSystem* DamageNumber_FX;
	UTP_WeaponComponent* WeaponRef;

};
