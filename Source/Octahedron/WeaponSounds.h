// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponSounds.generated.h"

class UTP_WeaponComponent;
class UMetaSoundSource;
class UAudioComponent;

UCLASS()
class OCTAHEDRON_API AWeaponSounds : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponSounds();

protected:
	// Called when the game starts or when spawned
	//virtual void BeginPlay() override;

	FTimerHandle CheckDestroyEffectTimerHandle;
	void CheckDestroyEffect();

	UTP_WeaponComponent* WeaponRef;
	float FireTime;
	UAudioComponent* FireAudioComp;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	void WeaponFire(
		UTP_WeaponComponent* inWeaponRef,
		float inFireTime
	);
	UMetaSoundSource* FireSound;
};
