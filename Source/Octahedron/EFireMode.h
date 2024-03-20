// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EFireMode.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EFireMode : uint8
{
    Single,
    Burst,
    Auto
};
