// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GenericTeamAgentInterface.h"
#include "WeaponPickup.generated.h"


UENUM(BlueprintType)
enum class EWeaponPickupRarity : uint8
{
	LEGENDARY,
	MASTER,
	RARE,
	COMMON
};

UCLASS()
class ADVGAMESPROGRAMMING_API AWeaponPickup : public APickup, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	AWeaponPickup();

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	EWeaponPickupRarity Rarity;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	float BulletDamage;
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	float MuzzleVelocity;
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	int32 MagazineSize;
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	float WeaponAccuracy;

	UFUNCTION(BlueprintImplementableEvent)
	void OnPickup(AActor* ActorThatPickedUp) override;
	UFUNCTION(BlueprintCallable)
	void OnGenerate() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void GenerateRandomBoolArray(int32 ArrayLength, int32 NumTrue, TArray<bool>& RandBoolArray);

	//Set up team id for the Weapon Pickup
	FGenericTeamId TeamId;
	virtual FGenericTeamId GetGenericTeamId() const override;
};
