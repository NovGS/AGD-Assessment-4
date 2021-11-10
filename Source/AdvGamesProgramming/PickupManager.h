// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "PickupManager.generated.h"



UCLASS()
class ADVGAMESPROGRAMMING_API APickupManager : public AActor
{
	GENERATED_BODY()

private:

	const float WEAPON_PICKUP_LIFETIME = 20.0f;
	const float HEALTH_PICKUP_LIFETIME = 30.0f;

	TArray<FVector> PossibleSpawnLocations;
	
	TSubclassOf<class APickup> WeaponPickupClass;
	float WeaponSpawnInterval;
	FTimerHandle WeaponSpawnTimer;

	void SpawnWeaponPickup();

	TSubclassOf<class AHealthPickup> HealthPickupClass;
	float HealthSpawnInterval;
	FTimerHandle HealthSpawnTimer;

	void SpawnHealthPickup();

public:	

	// Sets default values for this actor's properties
	APickupManager();

	void Init(const TArray<FVector>& PossibleSpawnLocationsArg, 
				TSubclassOf<APickup> WeaponPickupClassArg, 
				float WeaponSpawnIntervalArg, TSubclassOf<AHealthPickup> HealthPickupClassArg, float HealthSpawnIntervalArg);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
