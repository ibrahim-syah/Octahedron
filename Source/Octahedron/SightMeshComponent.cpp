// Fill out your copyright notice in the Description page of Project Settings.


#include "SightMeshComponent.h"

// Sets default values for this component's properties
USightMeshComponent::USightMeshComponent()
{
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	BoundsScale = 2.f;
}
