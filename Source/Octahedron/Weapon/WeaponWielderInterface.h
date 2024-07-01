// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../TP_WeaponComponent.h"
#include "../Public/EAmmoType.h"
#include "WeaponWielderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UWeaponWielderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class OCTAHEDRON_API IWeaponWielderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void AttachWeapon(UTP_WeaponComponent* Weapon);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void DetachWeapon();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	bool InstantDetachWeapon();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnEquip();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnStow();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	int32 GetRemainingAmmo(EAmmoType AmmoType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	int32 SetRemainingAmmo(EAmmoType AmmoType, int32 newValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	FVector GetTraceStart();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	FVector GetTraceForward();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	FRotator GetWielderControlRotation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void SetWielderControlRotation(FRotator &newRotator);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnWeaponFired();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnWeaponReload();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnWeaponStopReloadAnimation(float blendTime);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnSetIsReloadingFalse();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnADSTLUpdate(float TLValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void OnFinishPlay();
};
