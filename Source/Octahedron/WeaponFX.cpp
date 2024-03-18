// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponFX.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

// Sets default values
AWeaponFX::AWeaponFX()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneComponent"));
	SetRootComponent(DefaultSceneComponent);
}

// Called when the game starts or when spawned
void AWeaponFX::BeginPlay()
{
	Super::BeginPlay();
}

void AWeaponFX::BeginPlay(
	USkeletalMeshComponent* InWeaponMesh,
	UNiagaraSystem* InMuzzleFlash_FX,
	UNiagaraSystem* InTracer_FX,
	UNiagaraSystem* InShellEject_FX,
	UStaticMesh* InShellEjectMesh,
	FName* InMuzzleSocket,
	FName* InShellEjectSocket
)
{
	/*if (InWeaponMesh == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("WeaponMesh for the effect actor is missing!"));
		Destroy();
		return;
	}

	if (InMuzzleFlash_FX == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("MuzzleFlash_FX for the effect actor is missing!"));
		Destroy();
		return;
	}

	if (InTracer_FX == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Tracer_FX for the effect actor is missing!"));
		Destroy();
		return;
	}

	if (InShellEjectMesh == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("ShellEjectMesh for the effect actor is missing!"));
		Destroy();
		return;
	}

	if (InMuzzleSocket == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("MuzzleSocket for the effect actor is missing!"));
		Destroy();
		return;
	}

	if (InShellEjectSocket == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("ShellEjectSocket for the effect actor is missing!"));
		Destroy();
		return;
	}*/

	WeaponMesh = InWeaponMesh;
	MuzzleFlash_FX = InMuzzleFlash_FX;
	Tracer_FX = InTracer_FX;
	ShellEject_FX = InShellEject_FX;
	ShellEjectMesh = InShellEjectMesh;
	MuzzleSocket = InMuzzleSocket;
	ShellEjectSocket = InShellEjectSocket;
}

void AWeaponFX::WeaponFire(TArray<FVector> InImpactPositions)
{
	ImpactPositions = InImpactPositions;

	FVector MuzzleLocation = WeaponMesh->GetSocketTransform(*MuzzleSocket, ERelativeTransformSpace::RTS_Actor).GetLocation();
	// Shell Eject FX
	if (IsValid(ShellEject_FX))
	{
		FVector ShellEjectLocation = WeaponMesh->GetSocketTransform(*ShellEjectSocket, ERelativeTransformSpace::RTS_Actor).GetLocation();
		if (!IsValid(NC_ShellEject) || !NC_ShellEject->IsActive())
		{
			NC_ShellEject = UNiagaraFunctionLibrary::SpawnSystemAttached(
				ShellEject_FX,
				GetRootComponent(),
				NAME_None,
				ShellEjectLocation,
				FRotator(0.f, 90.f, 0.f),
				EAttachLocation::Type::KeepRelativeOffset,
				true);
			NC_ShellEject->SetNiagaraVariableObject(FString("ShellEjectStaticMesh"), ShellEjectMesh);

			ShellEjectTrigger = false;
		}
		
		ShellEjectTrigger = !ShellEjectTrigger;

		NC_ShellEject->SetNiagaraVariableBool(FString("Trigger"), ShellEjectTrigger);
	}

	// Muzzle Flash FX
	if (IsValid(MuzzleFlash_FX))
	{

		if (!IsValid(NC_MuzzleFlash) || !NC_MuzzleFlash->IsActive())
		{
			NC_MuzzleFlash = UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlash_FX,
				GetRootComponent(),
				NAME_None,
				MuzzleLocation,
				FRotator(0.f, 90.f, 0.f),
				EAttachLocation::Type::KeepRelativeOffset,
				true);

			MuzzleFlashTrigger = false;
		}

		MuzzleFlashTrigger = !MuzzleFlashTrigger;

		FVector MuzzleLocationWorld = WeaponMesh->GetSocketTransform(*MuzzleSocket, ERelativeTransformSpace::RTS_World).GetLocation();
		FVector NormalizedPositionDifference = (ImpactPositions[0] - MuzzleLocationWorld).GetSafeNormal(0.0001f);
		NC_MuzzleFlash->SetNiagaraVariableVec3(FString("Direction"), NormalizedPositionDifference);

		NC_MuzzleFlash->SetNiagaraVariableBool(FString("Trigger"), MuzzleFlashTrigger);
	}

	// Tracer FX
	if (IsValid(Tracer_FX))
	{
		if (!IsValid(NC_Tracer) || !NC_Tracer->IsActive())
		{
			NC_Tracer = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Tracer_FX,
				GetRootComponent(),
				NAME_None,
				MuzzleLocation,
				FRotator(0.f, 90.f, 0.f),
				EAttachLocation::Type::KeepRelativeOffset,
				true);
			TracerTrigger = false;
		}

		TracerTrigger = !TracerTrigger;

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NC_Tracer, FName("ImpactPositions"), ImpactPositions); // why unreal, whyyy
		NC_Tracer->SetNiagaraVariableBool(FString("Trigger"), TracerTrigger);
	}

	const float delay = 3.f;
	GetWorldTimerManager().SetTimer(CheckDestroyEffectTimerHandle, this, &AWeaponFX::CheckDestroyEffect, delay, true, delay);
}

void AWeaponFX::CheckDestroyEffect()
{
	if (IsValid(NC_MuzzleFlash) || IsValid(NC_ShellEject) || IsValid(NC_Tracer))
	{
		return;
	}
	else {
		Destroy();
	}
}

// Called every frame
//void AWeaponFX::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

