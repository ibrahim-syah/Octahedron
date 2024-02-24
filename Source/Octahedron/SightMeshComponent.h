// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "SightMeshComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OCTAHEDRON_API USightMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

	/** Sets default values for this component's properties */
	USightMeshComponent();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UMaterialInstance* FP_Material_Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UMaterialInstance* FP_Material_Holo;
	
};
