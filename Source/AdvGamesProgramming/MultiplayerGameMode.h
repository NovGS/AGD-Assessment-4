// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Procedural Dungeon Generation/MapGeneration.h"
#include "MultiplayerGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ADVGAMESPROGRAMMING_API AMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()

private:
	const float WEAPON_PICKUP_SPAWN_INTERVAL = 10.0f;
	const float HEALTH_PICKUP_SPAWN_INTERVAL = 15.0f;
	const int32 NUM_AI = 0;

	//class AProcedurallyGeneratedMap* ProceduralMap;
	class AMapGeneration* ProceduralMap;
	class APickupManager* PickupManager;
	class AAIManager* AIManager;

	int32 PlayerID;

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class APickup> WeaponPickupClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AHealthPickup> HealthPickupClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AEnemyCharacter> EnemyCharacterClass;

	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessages) override;
	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	void Respawn(AController* Controller);
	UFUNCTION()
	void TriggerRespawn(AController* Controller);

	int32 GetAndIncrementPlayerID();
};
