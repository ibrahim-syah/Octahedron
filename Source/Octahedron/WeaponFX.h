// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponFX.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class OCTAHEDRON_API AWeaponFX : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponFX();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneComponent = nullptr;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<FVector> ImpactPositions;

	UNiagaraComponent* NC_MuzzleFlash = nullptr;
	bool MuzzleFlashTrigger;
	UNiagaraComponent* NC_ShellEject = nullptr;
	bool ShellEjectTrigger;
	UNiagaraComponent* NC_Tracer = nullptr;
	bool TracerTrigger;

	FTimerHandle CheckDestroyEffectTimerHandle;
	void CheckDestroyEffect();

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	USkeletalMeshComponent* WeaponMesh = nullptr;
	UNiagaraSystem* MuzzleFlash_FX = nullptr;
	UNiagaraSystem* Tracer_FX = nullptr;
	UNiagaraSystem* ShellEject_FX = nullptr;
	UStaticMesh* ShellEjectMesh = nullptr;
	FName* MuzzleSocket = nullptr;
	FName* ShellEjectSocket = nullptr;

	UFUNCTION(BlueprintCallable)
	void WeaponFire(TArray<FVector> InImpactPositions);

};
