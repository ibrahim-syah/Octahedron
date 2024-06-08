// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomProjectile.generated.h"

class UBoxComponent;
class UArrowComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileImpact, AActor*, OtherActor, FHitResult, Hit);

UCLASS()
class OCTAHEDRON_API ACustomProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACustomProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	UBoxComponent* BoxCollision = nullptr;

	UPROPERTY(EditAnywhere, Category = Components)
	UArrowComponent* Arrow = nullptr;

	UPROPERTY(EditAnywhere, Category = Components)
	UStaticMeshComponent* Mesh = nullptr;

	UPROPERTY(EditAnywhere, Category = Components)
	UProjectileMovementComponent* ProjectileMovement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Settings", Meta = (ExposeOnSpawn = true))
	float Speed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Settings", Meta = (ExposeOnSpawn = true))
	float Gravity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Settings", Meta = (ExposeOnSpawn = true))
	bool IsHoming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Settings", Meta = (ExposeOnSpawn = true))
	float HomingAccelerationMagnitude = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Settings", Meta = (ExposeOnSpawn = true))
	bool ShouldRotateInitialVelocityToTarget = false;

	UPROPERTY(EditAnywhere, Category = Effects)
	UParticleSystem* ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = Sounds)
	USoundBase* ImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Target, Meta = (ExposeOnSpawn = true))
	AActor* Target = nullptr;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void RotateToTarget();

	UPROPERTY(BlueprintAssignable)
	FOnProjectileImpact OnProjectileImpactDelegate;

	UFUNCTION()
	void OnImpact(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);



};
