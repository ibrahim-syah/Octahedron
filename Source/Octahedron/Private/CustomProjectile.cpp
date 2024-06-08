// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"



// Sets default values
ACustomProjectile::ACustomProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(FName("BoxCollision"));
	SetRootComponent(BoxCollision);
	BoxCollision->SetCollisionProfileName(FName("BlockAllDynamic"));
	//BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//BoxCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	Arrow = CreateDefaultSubobject<UArrowComponent>(FName("Arrow"));
	Arrow->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh"));
	Mesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovement"));
	AddOwnedComponent(ProjectileMovement);

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = Gravity;
}

// Called when the game starts or when spawned
void ACustomProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Target) && IsHoming)
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
	}

	BoxCollision->OnComponentHit.AddDynamic(this, &ACustomProjectile::OnImpact);
	
	// Don't forget to ignore projectile from moving for the owner as well!
	BoxCollision->IgnoreActorWhenMoving(
		IsValid(GetInstigator()) ? GetInstigator() : GetOwner(),
		true
	);

	if (ShouldRotateInitialVelocityToTarget && IsValid(Target))
	{
		RotateToTarget();
	}
}

// Called every frame
void ACustomProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACustomProjectile::RotateToTarget()
{
	FVector newVelocity = UKismetMathLibrary::GetDirectionUnitVector(
		GetActorLocation(),
		Target->GetActorLocation()
	);
	ProjectileMovement->Velocity = newVelocity;
}

void ACustomProjectile::OnImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	OnProjectileImpactDelegate.Broadcast(OtherActor, Hit);

	if (IsValid(ImpactEffect)) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, Hit.Location, FRotator::ZeroRotator, true);
	}

	if (IsValid(ImpactSound)) {
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, Hit.Location);
	}

	Destroy();
}

