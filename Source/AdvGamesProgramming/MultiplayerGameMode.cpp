// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerGameMode.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "PickupManager.h"
#include "ProcedurallyGeneratedMap.h"
#include "Engine/Engine.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "PlayerHUD.h"
#include "PlayerCharacter.h"
#include "AIManager.h"
#include "GameFramework/PlayerController.h"

void AMultiplayerGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessages)
{
	Super::InitGame(MapName, Options, ErrorMessages);

	for (TActorIterator<AMapGeneration> It(GetWorld()); It; ++It)
	{
		ProceduralMap = *It;
	}

	TArray<FVector> Vertices;

	// Log a warning if the procedural map does not exist.
	if (!ProceduralMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Procedural Map found in the level"))
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Procedural Map found"))
		for (auto It = ProceduralMap->ConnectedTiles.CreateIterator(); It; ++It)
		{
			Vertices.Add(It->Key * FVector(300, 300, 0));
		}
	}

	PickupManager = GetWorld()->SpawnActor<APickupManager>();
	if (!PickupManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to spawn the APickupManager"))
	}

	if (ProceduralMap && PickupManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Vertices: %d"), Vertices.Num());
		if (!Vertices.Num() == 0)
		{
			PickupManager->Init(Vertices, WeaponPickupClass, WEAPON_PICKUP_SPAWN_INTERVAL, HealthPickupClass, HEALTH_PICKUP_SPAWN_INTERVAL);
		}
	}

	AIManager = GetWorld()->SpawnActor<AAIManager>();
	if (!AIManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to spawn the APickupManager"))
	}

	if (ProceduralMap && AIManager)
	{
		AIManager->Init(EnemyCharacterClass, NUM_AI, ProceduralMap->BlueSpawn, ProceduralMap->RedSpawn);
	}

	PlayerID = 0;

}

void AMultiplayerGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("In HandleStartingNewPlayer"));

	APlayerCharacter* Character = Cast<APlayerCharacter>(NewPlayer->GetPawn());
	if (Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player Character Found"));
	}
}



void AMultiplayerGameMode::Respawn(AController* Controller)
{
	if (Controller)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, FString::Printf(TEXT("Respawning")));
		}

		//Hide the hud
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if (PlayerController)
		{
			APlayerCharacter* Character = Cast<APlayerCharacter>(PlayerController->GetPawn());
			if (Character)
			{
				Character->SetPlayerHUDVisibility(false);
			}
		}

		//Remove the player
		APawn* Pawn = Controller->GetPawn();
		if (Pawn)
		{
			Pawn->SetLifeSpan(0.1f);
		}

		//Trigger the respawn process

		FTimerHandle RespawnTimer;
		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUFunction(this, TEXT("TriggerRespawn"), Controller);
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, 5.0f, false);
	}
}

void AMultiplayerGameMode::TriggerRespawn(AController* Controller)
{
	if (Controller)
	{
		AActor* SpawnPoint = ChoosePlayerStart(Controller);
		if (SpawnPoint)
		{
			APawn* SpawnedPlayer = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation());
			if (SpawnedPlayer)
			{
				Controller->Possess(SpawnedPlayer);
			}
		}
	}

	//Show the hud
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		APlayerCharacter* Character = Cast<APlayerCharacter>(PlayerController->GetPawn());
		if (Character)
		{
			Character->SetPlayerHUDVisibility(true);
		}
	}
}

int32 AMultiplayerGameMode::GetAndIncrementPlayerID()
{
	int32 OldPlayerID = PlayerID;
	PlayerID = (PlayerID == 0) ? 1 : 0;
	return OldPlayerID;
}


