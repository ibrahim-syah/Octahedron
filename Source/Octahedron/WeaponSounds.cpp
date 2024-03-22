// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSounds.h"
#include "MetasoundSource.h"
#include "TP_WeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

// Sets default values
AWeaponSounds::AWeaponSounds()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AWeaponSounds::WeaponFire()
{
	const float delay = 3.f;
	GetWorldTimerManager().SetTimer(CheckDestroyEffectTimerHandle, this, &AWeaponSounds::CheckDestroyEffect, delay, true, delay);

	if (FireSound != nullptr)
	{
		if (!IsValid(FireAudioComp) || !FireAudioComp->IsActive())
		{
			FireAudioComp = UGameplayStatics::SpawnSoundAttached(
				FireSound,
				WeaponRef
			);

			FireAudioComp->SetFloatParameter(FName("ShotInterval"), FireSoundInterval);
		}
		FireAudioComp->SetTriggerParameter(FName("Fire"));
	}
}

void AWeaponSounds::CheckDestroyEffect()
{
	if (IsValid(FireAudioComp))
	{
		return;
	}
	else {
		Destroy();
	}
}

// Called when the game starts or when spawned
void AWeaponSounds::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
//void AWeaponSounds::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

