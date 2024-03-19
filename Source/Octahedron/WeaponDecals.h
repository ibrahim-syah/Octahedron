// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponDecals.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class OCTAHEDRON_API AWeaponDecals : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponDecals();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpactNormals;
	TArray<int32> ImpactSurfaces;
	FVector MuzzlePosition;

	void RemoveInvalidSurfaces();
	UNiagaraComponent* NC_ImpactDecals;
	bool ImpactDecalsTrigger;

	TArray<int32> ImpactSurfacesInt;

	FTimerHandle CheckDestroyEffectTimerHandle;
	void CheckDestroyEffect();

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void WeaponFire(
		TArray<FVector> InImpactPositions,
		TArray<FVector> InImpactNormals,
		TArray<int32> InImpactSurfaces,
		FVector InMuzzlePosition
	);

	UNiagaraSystem* ImpactDecals_FX;

};
