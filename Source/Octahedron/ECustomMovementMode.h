// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ECustomMovementMode.generated.h"

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
    Walking,
    Crouching,
    Sprinting,
    Sliding
};
