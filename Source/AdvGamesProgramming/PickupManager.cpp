// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupManager.h"
#include "Engine/World.h"
#include "Pickup.h"
#include "HealthPickup.h"
#include "Engine/Engine.h"

// Sets default values
APickupManager::APickupManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void APickupManager::Init(const TArray<FVector>& PossibleSpawnLocationsArg, TSubclassOf<APickup> WeaponPickupClassArg, float WeaponSpawnIntervalArg, TSubclassOf<AHealthPickup> HealthPickupClassArg, float HealthSpawnIntervalArg)
{
	this->PossibleSpawnLocations = PossibleSpawnLocationsArg;
	
	this->WeaponPickupClass = WeaponPickupClassArg;
	this->WeaponSpawnInterval = WeaponSpawnIntervalArg;
	
	this->HealthPickupClass = HealthPickupClassArg;
	this->HealthSpawnInterval = HealthSpawnIntervalArg;
}

void APickupManager::SpawnHealthPickup()
{
	int32 RandomLocation = FMath::RandRange(0, PossibleSpawnLocations.Num() - 1);
	AHealthPickup* HealthPickup = GetWorld()->SpawnActor<AHealthPickup>(HealthPickupClass, PossibleSpawnLocations[RandomLocation] + FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator);
	HealthPickup->SetLifeSpan(HEALTH_PICKUP_LIFETIME);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Health Pickup Spawned :)")));
	}
}

void APickupManager::SpawnWeaponPickup()
{
	int32 RandomLocation = FMath::RandRange(0, PossibleSpawnLocations.Num() - 1);
	APickup* WeaponPickup = GetWorld()->SpawnActor<APickup>(WeaponPickupClass, PossibleSpawnLocations[RandomLocation] + FVector(0.0f,0.0f,50.0f), FRotator::ZeroRotator);
	WeaponPickup->SetLifeSpan(WEAPON_PICKUP_LIFETIME);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Pickup Spawned :)")));
	}
}

// Called when the game starts or when spawned
void APickupManager::BeginPlay()
{
	Super::BeginPlay();
	
	for (int i = 0; i < (PossibleSpawnLocations.Num()/50); i++)
	{
		SpawnWeaponPickup();
	}
	GetWorldTimerManager().SetTimer(WeaponSpawnTimer, this, &APickupManager::SpawnWeaponPickup, WeaponSpawnInterval, true, 0.0f);

	GetWorldTimerManager().SetTimer(HealthSpawnTimer, this, &APickupManager::SpawnHealthPickup, HealthSpawnInterval, true, 15.0f);
}

// Called every frame
void APickupManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

