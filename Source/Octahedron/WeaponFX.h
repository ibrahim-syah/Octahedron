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
	USceneComponent* DefaultSceneComponent;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void BeginPlay(
		USkeletalMeshComponent* InWeaponMesh,
		UNiagaraSystem* InMuzzleFlash_FX,
		UNiagaraSystem* InTracer_FX,
		UNiagaraSystem* InShellEject_FX,
		UStaticMesh* InShellEjectMesh,
		FName* InMuzzleSocket,
		FName* InShellEjectSocket
	);

	TArray<FVector> ImpactPositions;

	UNiagaraComponent* NC_MuzzleFlash;
	bool MuzzleFlashTrigger;
	UNiagaraComponent* NC_ShellEject;
	bool ShellEjectTrigger;
	UNiagaraComponent* NC_Tracer;
	bool TracerTrigger;

	FTimerHandle CheckDestroyEffectTimerHandle;
	void CheckDestroyEffect();

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	USkeletalMeshComponent* WeaponMesh;
	UNiagaraSystem* MuzzleFlash_FX;
	UNiagaraSystem* Tracer_FX;
	UNiagaraSystem* ShellEject_FX;
	UStaticMesh* ShellEjectMesh;
	FName* MuzzleSocket;
	FName* ShellEjectSocket;

	UFUNCTION(BlueprintCallable)
	void WeaponFire(TArray<FVector> InImpactPositions);

};
