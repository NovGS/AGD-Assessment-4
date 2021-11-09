// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Room.h"
#include "RoomConstruction.h"
#include "Directions.h"
#include "MapGeneration.generated.h"

UCLASS()
class ADVGAMESPROGRAMMING_API AMapGeneration : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapGeneration();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual bool ShouldTickIfViewportsOnly() const override;

	// Size of the rooms
	UPROPERTY(EditAnywhere, Category = "Room Parameters")
		int32 RoomMinSize;
	UPROPERTY(EditAnywhere, Category = "Room Parameters")
		int32 RoomMaxSize;
	
	// Minimum amount of wall space between each door
	UPROPERTY(EditAnywhere, Category = "Room Parameters")
		int32 DoorSpacing;
	
	// Number of rooms
	UPROPERTY(EditAnywhere, Category = "Level Parameters")
		int32 RoomNumMin;
	UPROPERTY(EditAnywhere, Category = "Level Parameters")
		int32 RoomNumMax;
	
	int32 RoomNum;

	UPROPERTY(EditAnywhere)
		bool bRegenerateMap;
	
	// The bottom left corner of the initial room spawn
	UPROPERTY(EditAnywhere, Category = "Level Parameters")
		FVector FirstRootLocation;

	// Meshes used for construction
	UPROPERTY(EditAnywhere, Category = "Room Construction")
		TSubclassOf<class ARoomConstruction> Wall;
	UPROPERTY(EditAnywhere, Category = "Room Construction")
		TSubclassOf<class ARoomConstruction> Tile;
	UPROPERTY(EditAnywhere, Category = "Room Construction")
		TSubclassOf<class ARoomConstruction> Doorway;

	UPROPERTY(VisibleAnywhere, Category = "Navigation Nodes")
		TArray<class ANavigationNode*> AllNodes;

	UPROPERTY(EditAnywhere, Category = "Agents")
		TSubclassOf<class AEnemyCharacter> AgentToSpawn;

	UPROPERTY(EditAnywhere, Category = "Player Starts")
		TSubclassOf<class APlayerStart> PlayerStartClass;
	UPROPERTY(VisibleAnywhere, Category = "Player Starts")
		TArray<ANavigationNode*> BlueSpawn;
	UPROPERTY(VisibleAnywhere, Category = "Player Starts")
		TArray<ANavigationNode*> RedSpawn;

	// Array of all rooms in the level
	TArray<Room> Rooms;

	// Map of all tiles which have been validated to allow for collisiion detection
	UPROPERTY(VisibleAnywhere)
	TMap<FVector, int32> ConnectedTiles;

	void GenerateLevel();

	Room ChooseValidRoomPlacement();
	TArray<int32> TestRoomAgainstConnectedTiles(Room* RoomToTest);
	void SetBothDoors(Room* RoomToSpawnDoor, Room* OtherRoomToSpawnDoor, FVector Door, EDirections DirectionToBuild);
	void SetDoor(Room* RoomToSpawnDoor, FVector Door, EDirections DirectionToBuild);
	FVector OpposingDoor(FVector DoorToBuildFrom, EDirections DirectionToBuild);
	void RemoveDoorsFromAvailableWallTiles(Room* RoomToRemoveDoors, FVector Door, EDirections DirectionToBuild);
	
	void GenerateRoom(Room* RoomToGenerate);

	void SpawnRoom(Room RoomToSpawn);

	void FindSpawnRooms(TArray<Room*>* BlueSpawnRooms, TArray<Room*>* RedSpawnRooms);
	void FindLimitsOfMap(int32* MinX, int32* MaxX, int32* MinY, int32* MaxY);

private:

	// Scaling of meshes from the FVectors used
	FVector Scale;

	// Z value to mark FVectors as invalid
	int32 InvalidZValue;

	// Mark whether the algorithm can no longer find valid rooms for placement
	bool bNoValidRooms;

	void PopulateCorners(Room* RoomToGenerate);
	void PopulateWalls(Room* RoomToGenerate);
	void PopulateAvailableWalls(Room* RoomToGenerate);
	void PopulateInnerTiles(Room* RoomToGenerate);

	FVector2D CalculateRoomSize(int32 RoomMinSize, int32 RoomMaxSize);
	TArray<int32> GenerateShuffledRoomsIndex();
	TArray<FVector> ShuffleFVectorArr(TArray<FVector> ArrayToShuffle);
	FVector GenerateRandomizedRootTile(Room* RoomToBranchFrom, FVector2D NewRoomSize, FVector Door, EDirections* DirectionToBuild);

	void AddRoomToRooms(Room RoomToAdd, int32 RoomIndex);

	void GenerateNodes();
	void ConnectNodes();
	void AddConnection(ANavigationNode* FromNode, ANavigationNode* ToNode);

	void ClearMap();

	void SpawnTeams();
	TArray<ANavigationNode*> GenerateSpawnNodes(Room* SpawnRoom);
	FVector RandomDoorVector(Room* SpawnRoom);
};
