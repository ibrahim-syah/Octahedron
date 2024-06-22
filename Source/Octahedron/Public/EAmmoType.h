// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EAmmoType.generated.h"

/**
 *
 */
UENUM(BlueprintType)
enum class EAmmoType : uint8
{
    None,
    Primary,
    Special,
    Heavy
};
