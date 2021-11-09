// Fill out your copyright notice in the Description page of Project Settings.


#include "MapGeneration.h"
#include "Engine/World.h"
#include "../NavigationNode.h"
#include "../EnemyCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AMapGeneration::AMapGeneration()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RoomMinSize = 2;
	RoomMaxSize = 5;
	
	RoomNumMin = 5;
	RoomNumMax = 10;
	RoomNum = 1;
	
	Scale = FVector(300, 300, 0);
	
	FirstRootLocation = FVector(0, 0, 0);
	
	DoorSpacing = 2;
	InvalidZValue = -1;
	bNoValidRooms = false;
}

// Called when the game starts or when spawned
void AMapGeneration::BeginPlay()
{
	Super::BeginPlay();
}

// Allows map to be generated upon ticking RegenerateMap in the editor
void AMapGeneration::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bRegenerateMap)
	{
		ClearMap();
		RoomNum = FMath::RandRange(RoomNumMin, RoomNumMax);
		InvalidZValue = FirstRootLocation.Z - 1;

		GenerateLevel();
		GenerateNodes();
		ConnectNodes();
		SpawnTeams();

		bRegenerateMap = false;
	}
}

bool AMapGeneration::ShouldTickIfViewportsOnly() const
{
	return true;
}

// Generates the map to be spawned
void AMapGeneration::GenerateLevel()
{
	int32 CurrentRoomIndex = 0;
	// Generate FirstRoom at chosen RootLocation
	Room FirstRoom;
	FirstRoom.RoomSize = CalculateRoomSize(RoomMinSize, RoomMaxSize);
	FirstRoom.CornerTiles.Add(FirstRootLocation);
	
	GenerateRoom(&FirstRoom);
	AddRoomToRooms(FirstRoom, CurrentRoomIndex);
	
	// Generate Rooms until required number of rooms is reached or no more rooms can be placed
	while (Rooms.Num() < RoomNum)
	{
		CurrentRoomIndex++;
		Room NewRoom = ChooseValidRoomPlacement();
		if (bNoValidRooms)
		{
			break;
		}
		AddRoomToRooms(NewRoom, CurrentRoomIndex);
	}

	// Spawn all rooms
	for (auto It = Rooms.CreateIterator(); It; ++It) {
		SpawnRoom(*It);
	}
}

// Chooses and tests for a new valid room
Room AMapGeneration::ChooseValidRoomPlacement()
{
	//bRestart tracks whether or not the searching should be restarted due to
	//generating a door without generating a room
	bool bRestart = false;
	Room TestRoom;
	
	while (true)
	{
		bRestart = false;
		while (!bRestart)
		{
			// Generate new RoomSize
			FVector2D NewRoomSize = CalculateRoomSize(RoomMinSize, RoomMaxSize);

			/*If the algorithm found no valid rooms for the above randomly generated room size,
			retry the algorithm with the smallest room size*/
			if (bNoValidRooms)
			{
				NewRoomSize = FVector2D(RoomMinSize, RoomMinSize);
			}

			// Generates an array with the indexes of available rooms shuffled
			TArray<int32> ShuffledRoomIdxs = GenerateShuffledRoomsIndex();

			// Work through randomised rooms until valid room is successfully generated
			for (int i = 0; i < ShuffledRoomIdxs.Num(); i++)
			{
				Room* RoomToBranchFrom = &Rooms[ShuffledRoomIdxs[i]];
				TArray<FVector> AvailableDoorPlacements;

				/*Add only valid tiles from AvailableWallTiles to the AvailableDoorPlacements
				Invalid Z value is a way of identifying an invalid tile*/
				for (int j = 0; j < RoomToBranchFrom->AvailableWallTiles.Num(); j++)
				{
					if (RoomToBranchFrom->AvailableWallTiles[j].Z > InvalidZValue)
					{
						AvailableDoorPlacements.Add(RoomToBranchFrom->AvailableWallTiles[j]);
					}
				}

				// Only shuffle array if there are tiles available for a doorspace
				if (AvailableDoorPlacements.Num() != 0)
				{
					AvailableDoorPlacements = ShuffleFVectorArr(AvailableDoorPlacements);
				}

				// Work through randomised door placements until valid room is successfully generated
				for (int j = 0; j < AvailableDoorPlacements.Num(); j++)
				{
					EDirections DirectionToBuild;
					
					// Clear the TestRoom to accommodate for retrying of the loop
					TestRoom.ClearRoom();
					TestRoom.RoomSize = NewRoomSize;

					// Add root tile to TestRoom to allow room generation
					TestRoom.CornerTiles.Add(GenerateRandomizedRootTile(RoomToBranchFrom, TestRoom.RoomSize, AvailableDoorPlacements[j], &DirectionToBuild));

					// Generate a full room so that all tiles are populated
					GenerateRoom(&TestRoom);

					// Check whether or not TestRoom tiles intersect with existing tiles
					TArray<int32> IntersectingRooms = TestRoomAgainstConnectedTiles(&TestRoom);

					// If no intersections, then create a room, set doors, and return the valid room
					if (IntersectingRooms.Num() == 0)
					{
						SetBothDoors(RoomToBranchFrom, &TestRoom, AvailableDoorPlacements[j], DirectionToBuild);
						bNoValidRooms = false;
						return TestRoom;
						break;
					}
					else {
						/*Test whether any of the intersecting rooms are right next to the doorspace selected
						and if so, generate a Doorway and restart the testing with a new Room*/
						FVector DoorToFind = OpposingDoor(AvailableDoorPlacements[j], DirectionToBuild);
						for (int k = 0; k < IntersectingRooms.Num(); k++)
						{
							if (Rooms[IntersectingRooms[k]].AvailableWallTiles.Contains(DoorToFind))
							{
								SetBothDoors(RoomToBranchFrom, &Rooms[IntersectingRooms[k]], AvailableDoorPlacements[j], DirectionToBuild);
								bRestart = true;
								break;
							}
						}

					}
					if (bRestart) { break; }
				}
				if (bRestart) { break; }

				/* If already looped through twice with a randomised room size and the minimum room size
				 then return, spawn all Rooms added and end the algorithm*/
				if (bNoValidRooms) { return TestRoom; }

				// Set bNoValidRooms to true if it is the final loop
				if (i == ShuffledRoomIdxs.Num() - 1)
				{
					bNoValidRooms = true;
				}
			}
		}
	}
	return TestRoom;
}

 //Tests if Room intersects with existing room
 //Returns array of index of Rooms it intersects with
TArray<int32> AMapGeneration::TestRoomAgainstConnectedTiles(Room* RoomToTest)
{
	TArray<int32> IntersectingRoomIndexes;

	// Checking CornerTiles for intersections
	for (int i = 0; i < RoomToTest->CornerTiles.Num(); i++) 
	{
		if (ConnectedTiles.Contains(RoomToTest->CornerTiles[i]))
		{
			IntersectingRoomIndexes.Add(*ConnectedTiles.Find(RoomToTest->CornerTiles[i]));
		}
	}

	// Checking WallTiles for intersections
	for (auto It = RoomToTest->WallTiles.CreateIterator(); It; ++It)
	{
		if (ConnectedTiles.Contains(It->Key))
		{
			IntersectingRoomIndexes.Add(*ConnectedTiles.Find(It->Key));
		}
	}

	// Checking RoomTiles for intersections
	for (auto It = RoomToTest->RoomTiles.CreateIterator(); It; ++It)
	{
		if (ConnectedTiles.Contains(It->Key))
		{
			IntersectingRoomIndexes.Add(*ConnectedTiles.Find(It->Key));
		}
	}
	return IntersectingRoomIndexes;
}

void AMapGeneration::SetBothDoors(Room* RoomToSpawnDoor, Room* OtherRoomToSpawnDoor, FVector Door, EDirections DirectionToBuild)
{
	// Set door for original room and then remove the surrounding AvailableWallTiles for a range of DoorSpace
	SetDoor(RoomToSpawnDoor, Door, DirectionToBuild);
	RemoveDoorsFromAvailableWallTiles(RoomToSpawnDoor, Door, DirectionToBuild);
	
	// Set door for the opposing room

	// Find the FVector of the opposing door, set door, and then remove surround AvailableWallTiles
	FVector OtherDoor = OpposingDoor(Door, DirectionToBuild);

	switch (DirectionToBuild)
	{
	case EDirections::W:
	{
		SetDoor(OtherRoomToSpawnDoor, OtherDoor, EDirections::E);
		RemoveDoorsFromAvailableWallTiles(OtherRoomToSpawnDoor, OtherDoor, EDirections::E);
		break;
	}
	case EDirections::N:
	{
		SetDoor(OtherRoomToSpawnDoor, OtherDoor, EDirections::S);
		RemoveDoorsFromAvailableWallTiles(OtherRoomToSpawnDoor, OtherDoor, EDirections::S);
		break;
	}
	case EDirections::E:
	{
		SetDoor(OtherRoomToSpawnDoor, OtherDoor, EDirections::W);
		RemoveDoorsFromAvailableWallTiles(OtherRoomToSpawnDoor, OtherDoor, EDirections::W);
		break;
	}
	case EDirections::S:
	{
		SetDoor(OtherRoomToSpawnDoor, OtherDoor, EDirections::N);
		RemoveDoorsFromAvailableWallTiles(OtherRoomToSpawnDoor, OtherDoor, EDirections::N);
		break;
	}
	}
}

// Sets the Enum of the Door tile to be DOORWAY
void AMapGeneration::SetDoor(Room* RoomToSpawnDoor, FVector Door, EDirections DirectionToBuild)
{
	// Checking if Door FVector exists in corner tiles
	if (RoomToSpawnDoor->CornerTiles.Find(Door) >= 0)
	{
		// Remove the map for that FVector, storing the Enum array
		TArray<ERoomConstructionType> Sides = RoomToSpawnDoor->CornerTilesMap.FindAndRemoveChecked(Door);
		
		// Find the side that the Doorway should be built on and set Sides array
		switch (DirectionToBuild)
		{
		case EDirections::W:
		{
			if (Door == RoomToSpawnDoor->CornerTiles[0])
			{
				Sides[1] = ERoomConstructionType::DOORWAY;
			}
			else if (Door == RoomToSpawnDoor->CornerTiles[1])
			{
				Sides[0] = ERoomConstructionType::DOORWAY;
			}
			break;
		}
		case EDirections::N:
		{
			if (Door == RoomToSpawnDoor->CornerTiles[1])
			{
				Sides[1] = ERoomConstructionType::DOORWAY;
			}
			else if (Door == RoomToSpawnDoor->CornerTiles[2])
			{
				Sides[0] = ERoomConstructionType::DOORWAY;
			}
			break;
		}
		case EDirections::E:
		{
			if (Door == RoomToSpawnDoor->CornerTiles[2])
			{
				Sides[1] = ERoomConstructionType::DOORWAY;
			}
			else if (Door == RoomToSpawnDoor->CornerTiles[3])
			{
				Sides[0] = ERoomConstructionType::DOORWAY;
			}
			break;
		}
		case EDirections::S:
		{
			if (Door == RoomToSpawnDoor->CornerTiles[3])
			{
				Sides[1] = ERoomConstructionType::DOORWAY;
			}
			else if (Door == RoomToSpawnDoor->CornerTiles[0])
			{
				Sides[0] = ERoomConstructionType::DOORWAY;
			}
			break;
		}
		}
		// Add new Map entry with new Enum array
		RoomToSpawnDoor->CornerTilesMap.Add(Door, Sides);
	}
	// Removes and adds new Map entry with new Enum array if Door is a WallTile
	else if (RoomToSpawnDoor->WallTiles.Find(Door)){
		ERoomConstructionType ObjectToSpawn = RoomToSpawnDoor->WallTiles.FindAndRemoveChecked(Door);
		ObjectToSpawn = ERoomConstructionType::DOORWAY;
		RoomToSpawnDoor->WallTiles.Add(Door, ObjectToSpawn);
	}

}

// Finds the opposing door on the other side of the wall
FVector AMapGeneration::OpposingDoor(FVector DoorToBuildFrom, EDirections DirectionToBuild)
{
	switch (DirectionToBuild)
	{
	case EDirections::W:
	{
		return DoorToBuildFrom + FVector::LeftVector;
		break;
	}
	case EDirections::N:
	{
		return DoorToBuildFrom + FVector::ForwardVector;;
		break;
	}
	case EDirections::E:
	{
		return DoorToBuildFrom + FVector::RightVector;;
		break;
	}
	case EDirections::S:
	{
		return DoorToBuildFrom + FVector::BackwardVector;
		break;
	}
	}
	return FVector();
}

// Removes AvailableWallTiles according to the size of Doorspace, using indexes surrounding the Door position
void AMapGeneration::RemoveDoorsFromAvailableWallTiles(Room* RoomToRemoveDoors, FVector Door, EDirections DirectionToBuild)
{
	int32 DoorIndex = -1;
	
	// If Door is in a corner, manually set the index
	if (RoomToRemoveDoors->CornerTiles.Find(Door) >= 0)
	{
		switch (RoomToRemoveDoors->CornerTiles.Find(Door))
		{
		case 0: // Bottom Left corner
			DoorIndex = DirectionToBuild == EDirections::S ? 0 : 1;
			break;
		case 1: // Top Left corner
			DoorIndex = DirectionToBuild == EDirections::W ? RoomToRemoveDoors->RoomSize.X : RoomToRemoveDoors->RoomSize.X + 1;
			break;
		case 2: // Top Right corner
			DoorIndex = DirectionToBuild == EDirections::N ? RoomToRemoveDoors->RoomSize.X + RoomToRemoveDoors->RoomSize.Y : RoomToRemoveDoors->RoomSize.X + RoomToRemoveDoors->RoomSize.Y + 1;
			break;
		case 3: // Bottom Right corner
			DoorIndex = DirectionToBuild == EDirections::E ? (2 * RoomToRemoveDoors->RoomSize.X) + RoomToRemoveDoors->RoomSize.Y : (2 * RoomToRemoveDoors->RoomSize.X) + RoomToRemoveDoors->RoomSize.Y + 1;
			break;
		}
	}
	else {
		// Set index according to the index that the WallTile is found at
		for (int i = 0; i < RoomToRemoveDoors->AvailableWallTiles.Num(); i++)
		{
			if (RoomToRemoveDoors->AvailableWallTiles[i] == Door) {
				DoorIndex = i;
			}
		}
	}

	// Loop from indexes below the Door position to above, removing the AvailableWallTile by setting
	// its Z value to the invalid Z value, marking it as invalid
	for (int i = DoorIndex - DoorSpacing; i < DoorIndex + DoorSpacing + 1; i++)
	{
		if (i < 0)
		{
			RoomToRemoveDoors->AvailableWallTiles[RoomToRemoveDoors->AvailableWallTiles.Num() + i].Z = InvalidZValue;
		}
		else if (i >= RoomToRemoveDoors->AvailableWallTiles.Num())
		{
			RoomToRemoveDoors->AvailableWallTiles[i - RoomToRemoveDoors->AvailableWallTiles.Num()].Z = InvalidZValue;
		}
		else {
			RoomToRemoveDoors->AvailableWallTiles[i].Z = InvalidZValue;
		}
	}
}

// Generate an int array of shuffled indexes according to the Rooms array
TArray<int32> AMapGeneration::GenerateShuffledRoomsIndex()
{
	TArray<int32> ShuffledArr;

	for (int i = 0; i < Rooms.Num(); i++)
	{
		ShuffledArr.Add(i);
	}

	for (int i = 0; i < ShuffledArr.Num(); i++)
	{
		int RandIndex = FMath::RandRange(0, ShuffledArr.Num() - 1);
		int32 Temp = ShuffledArr[i];
		ShuffledArr[i] = ShuffledArr[RandIndex];
		ShuffledArr[RandIndex] = Temp;
	}
	return ShuffledArr;
}

// Shuffle an FVector array
TArray<FVector> AMapGeneration::ShuffleFVectorArr(TArray<FVector> ArrayToShuffle)
{
	TArray<FVector> ShuffledArr = ArrayToShuffle;

	for (int i = 0; i < ShuffledArr.Num(); i++)
	{
		int RandIndex = FMath::RandRange(0, ShuffledArr.Num() - 1);
		FVector Temp = ShuffledArr[i];
		ShuffledArr[i] = ShuffledArr[RandIndex];
		ShuffledArr[RandIndex] = Temp;
	}
	return ShuffledArr;
}

// Generate a root tile that will create a room that is adjacent to the required door
FVector AMapGeneration::GenerateRandomizedRootTile(Room* RoomToBranchFrom, FVector2D NewRoomSize, FVector Door, EDirections* DirectionToBuild)
{
	/* Check if Door is on a corner and if so, randomise the direction for that corner using availablewalltiles
	 that are also not Doorways*/
	if (RoomToBranchFrom->CornerTiles.Contains(Door))
	{
		int32 RoomSizeX = RoomToBranchFrom->RoomSize.X;
		int32 RoomSizeY = RoomToBranchFrom->RoomSize.Y;
		TArray<EDirections> PossibleDirections;
		switch (RoomToBranchFrom->CornerTiles.Find(Door))
		{
		case 0: // Bottom Left corner
			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[0] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[0].Z >= 0)
			{
				PossibleDirections.Add(EDirections::S);
			}

			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[1] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[1].Z >= 0)
			{
				PossibleDirections.Add(EDirections::W);
			}
			
			*DirectionToBuild = PossibleDirections[FMath::RandRange(0, PossibleDirections.Num()-1)];
			break;
		case 1: // Top Left corner
			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[0] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[RoomSizeX].Z >= 0)
			{
				PossibleDirections.Add(EDirections::W);
			}

			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[1] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[RoomSizeX + 1].Z >= 0)
			{
				PossibleDirections.Add(EDirections::N);
			}

			*DirectionToBuild = PossibleDirections[FMath::RandRange(0, PossibleDirections.Num()-1)];
			break;
		case 2: // Top Right corner
			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[0] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[RoomSizeX + RoomSizeY].Z >= 0)
			{
				PossibleDirections.Add(EDirections::N);
			}

			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[1] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[RoomSizeX + RoomSizeY + 1].Z >= 0)
			{
				PossibleDirections.Add(EDirections::E);
			}

			*DirectionToBuild = PossibleDirections[FMath::RandRange(0, PossibleDirections.Num()-1)];
			break;
		case 3: // Bottom Right corner
			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[0] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[(2*RoomSizeX) + RoomSizeY].Z >= 0)
			{
				PossibleDirections.Add(EDirections::E);
			}

			if ((*(RoomToBranchFrom->CornerTilesMap.Find(Door)))[1] != ERoomConstructionType::DOORWAY && RoomToBranchFrom->AvailableWallTiles[(2 * RoomSizeX) + RoomSizeY + 1].Z >= 0)
			{
				PossibleDirections.Add(EDirections::S);
			}

			*DirectionToBuild = PossibleDirections[FMath::RandRange(0, PossibleDirections.Num()-1)];
			break;
		}
	} // Door is on Left side of RoomToBranchFrom
	else if (Door.Y == RoomToBranchFrom->CornerTiles[0].Y){
		*DirectionToBuild = EDirections::W;
	} // Door is on Top side of RoomToBranchFrom
	else if (Door.X == RoomToBranchFrom->CornerTiles[1].X) {
		*DirectionToBuild = EDirections::N;
	} // Door is on Right side of RoomToBranchFrom
	else if (Door.Y == RoomToBranchFrom->CornerTiles[2].Y) {
		*DirectionToBuild = EDirections::E;
	} // Door is on Bottom side of RoomToBranchFrom
	else if (Door.X == RoomToBranchFrom->CornerTiles[0].X) {
		*DirectionToBuild = EDirections::S;
	}
	
	// Generate the FVector of the new root tile
	switch (*DirectionToBuild)
	{
	case EDirections::W:
	{
		int32 NewCornerMinX = Door.X - NewRoomSize.X + 1;
		int32 NewCornerMaxX = Door.X;
		int32 NewCornerY = Door.Y - NewRoomSize.Y;

		return FVector(FMath::RandRange(NewCornerMinX, NewCornerMaxX), NewCornerY, Door.Z);
		break;
	}
	case EDirections::N:
	{
		int32 NewCornerX = Door.X + 1;
		int32 NewCornerMinY = Door.Y - NewRoomSize.Y + 1;
		int32 NewCornerMaxY = Door.Y;

		return FVector(NewCornerX, FMath::RandRange(NewCornerMinY, NewCornerMaxY), Door.Z);
		break;
	}
	case EDirections::E:
	{
		int32 NewCornerMinX = Door.X - NewRoomSize.X + 1;
		int32 NewCornerMaxX = Door.X;
		int32 NewCornerY = Door.Y + 1;

		return FVector(FMath::RandRange(NewCornerMinX, NewCornerMaxX), NewCornerY, Door.Z);
		break;
	}
	case EDirections::S:
	{
		int32 NewCornerX = Door.X - NewRoomSize.X;
		int32 NewCornerMinY = Door.Y - NewRoomSize.Y + 1;
		int32 NewCornerMaxY = Door.Y;

		return FVector(NewCornerX, FMath::RandRange(NewCornerMinY, NewCornerMaxY), Door.Z);
		break;
	}
	}
	return FVector();
}

// Generate a room
// Requires a CornerTile[0] where the Room will generate from and a RoomSize
void AMapGeneration::GenerateRoom(Room* RoomToGenerate)
{
	PopulateCorners(RoomToGenerate);
	PopulateWalls(RoomToGenerate);
	PopulateAvailableWalls(RoomToGenerate);
	PopulateInnerTiles(RoomToGenerate);
}

// Populate CornerTiles and CornerTilesMap according to its RoomSize
void AMapGeneration::PopulateCorners(Room* RoomToGenerate)
{
	FVector2D Size = RoomToGenerate->RoomSize;
	FVector RootLocation = RoomToGenerate->CornerTiles[0];

	// Corners are added in correct order -> TopLeft, TopRight, BottomRight
	// First corner BottomLeft is added as RootLocation
	RoomToGenerate->CornerTiles.Add(FVector(Size.X-1, 0, 0) + RootLocation);
	RoomToGenerate->CornerTiles.Add(FVector(Size.X-1, Size.Y-1, 0) + RootLocation);
	RoomToGenerate->CornerTiles.Add(FVector(0, Size.Y-1, 0) + RootLocation);

	// Populate CornerTilesMap with only Walls
	TArray<ERoomConstructionType> WallArr;
	WallArr.Init(ERoomConstructionType::WALL, 2);

	for (int i = 0; i < 4; i++)
	{
		RoomToGenerate->CornerTilesMap.Add(RoomToGenerate->CornerTiles[i], WallArr);
	}
}

// Populate WallTiles according to RoomSize
void AMapGeneration::PopulateWalls(Room* RoomToGenerate)
{
	FVector2D Size = RoomToGenerate->RoomSize;

	// Generate Walled sides
	// Generate Left and Right sides
	for (int i = 1; i < Size.X - 1; i++)
	{
		RoomToGenerate->WallTiles.Add(RoomToGenerate->CornerTiles[0] + FVector(i, 0, 0), ERoomConstructionType::WALL);
		RoomToGenerate->WallTiles.Add(RoomToGenerate->CornerTiles[3] + FVector(i, 0, 0), ERoomConstructionType::WALL);
	}

	// Generate Top and Bottom sides
	for (int i = 1; i < Size.Y - 1; i++)
	{
		RoomToGenerate->WallTiles.Add(RoomToGenerate->CornerTiles[0] + FVector(0, i, 0), ERoomConstructionType::WALL);
		RoomToGenerate->WallTiles.Add(RoomToGenerate->CornerTiles[1] + FVector(0, i, 0), ERoomConstructionType::WALL);
	}
}

// Add AvailableWallTiles so that all tiles are in order, beginning from the Bottom Left southern border
void AMapGeneration::PopulateAvailableWalls(Room* RoomToGenerate)
{
	FVector2D Size = RoomToGenerate->RoomSize;

	// Bottom Left corner
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[0]);
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[0]);

	// Western walls
	for (int i = 1; i < Size.X - 1; i++)
	{
		RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[0] + FVector(i, 0, 0));
	}

	// Top Left corner
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[1]);
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[1]);

	// Northern walls
	for (int i = 1; i < Size.Y - 1; i++)
	{
		RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[1] + FVector(0, i, 0));
	}

	// Top Right corner
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[2]);
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[2]);

	// Eastern walls
	for (int i = 1; i < Size.X - 1; i++)
	{
		RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[2] - FVector(i, 0, 0));
	}

	// Bottom Right corner
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[3]);
	RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[3]);

	// Southern walls
	for (int i = 1; i < Size.Y - 1; i++)
	{
		RoomToGenerate->AvailableWallTiles.Add(RoomToGenerate->CornerTiles[3] - FVector(0, i, 0));
	}
}

// Populate RoomTiles according to RoomSize
void AMapGeneration::PopulateInnerTiles(Room* RoomToGenerate)
{
	FVector2D Size = RoomToGenerate->RoomSize;
	
	// Generate Inner tiles
	for (int i = 1; i < Size.X - 1; i++)
	{
		for (int j = 1; j < Size.Y - 1; j++)
		{
			RoomToGenerate->RoomTiles.Add(RoomToGenerate->CornerTiles[0] + FVector(i, j, 0), ERoomConstructionType::TILE);
		}
	}
}

// Spawn a Room
void AMapGeneration::SpawnRoom(Room RoomToSpawn)
{
	// Spawn all walls
	for (auto It = RoomToSpawn.WallTiles.CreateIterator(); It; ++It)
	{
		// Spawn a Tile mesh for all WallTiles
		GetWorld()->SpawnActor<ARoomConstruction>(Tile, It->Key * Scale, FRotator::ZeroRotator);
		
		// Rotation of the wall depending on which side it is on
		FRotator WallRotation;

		// Southern wall
		if (It->Key.X == RoomToSpawn.CornerTiles[0].X)
		{
			WallRotation = FRotator(0, 180, 0);
		}
		// Western wall
		else if (It->Key.Y == RoomToSpawn.CornerTiles[0].Y)
		{
			WallRotation = FRotator(0, -90, 0);
		}
		// Northern wall
		else if (It->Key.X == RoomToSpawn.CornerTiles[2].X)
		{
			WallRotation = FRotator(0, 0, 0);
		}
		// Eastern wall
		else if (It->Key.Y == RoomToSpawn.CornerTiles[2].Y)
		{
			WallRotation = FRotator(0, 90, 0);
		}

		// Spawn a wall or a doorway according to its Map value
		switch (It->Value)
		{
			case ERoomConstructionType::WALL:
				GetWorld()->SpawnActor<ARoomConstruction>(Wall, It->Key * Scale, WallRotation);
				break;
			case ERoomConstructionType::DOORWAY:
				GetWorld()->SpawnActor<ARoomConstruction>(Doorway, It->Key * Scale, WallRotation);
				break;
		}
	}

	// Spawn tiles for RoomTiles
	for (auto It = RoomToSpawn.RoomTiles.CreateIterator(); It; ++It)
	{
		GetWorld()->SpawnActor<ARoomConstruction>(Tile, It->Key * Scale, FRotator::ZeroRotator);
	}

	// Spawn all corners
	FRotator RotScale = FRotator(0, 90, 0);
	FRotator WallRotation = FRotator(0, 180, 0);

	// Spawns corners in order using the CornerTiles array, clockwise from Bottom Left
	for (auto It = RoomToSpawn.CornerTiles.CreateIterator(); It; ++It)
	{
		// Spawns a tile for all CornerTiles
		GetWorld()->SpawnActor<ARoomConstruction>(Tile, *(It) * Scale, FRotator::ZeroRotator);

		// Spawns two wall or doorway for each CornerTile
		for (int i = 0; i < 2; i++)
		{
			// Adjusts wall rotation
			if (i == 1)
			{
				WallRotation += RotScale;
			}

			// Finds Enum information from CornerTilesMap using the CornerTiles
			switch (RoomToSpawn.CornerTilesMap[*(It)][i])
			{
				case ERoomConstructionType::WALL:
					GetWorld()->SpawnActor<ARoomConstruction>(Wall, *(It) * Scale, WallRotation);
					break;
				case ERoomConstructionType::DOORWAY:
					GetWorld()->SpawnActor<ARoomConstruction>(Doorway, *(It) * Scale, WallRotation);
					break;
			}
		}
	}
}

// Calculate RoomSize
FVector2D AMapGeneration::CalculateRoomSize(int32 RoomMin, int32 RoomMax)
{
	return FVector2D(FMath::RandRange(RoomMin, RoomMax), FMath::RandRange(RoomMin, RoomMax));
}

// Add completed Room object to array of Room objects
// Add all tiles within that room to overall ConnectedTiles array
void AMapGeneration::AddRoomToRooms(Room RoomToAdd, int32 RoomIndex)
{
	Rooms.Add(RoomToAdd);

	// Adding RoomTiles to total ConnectedTiles
	for (auto It=RoomToAdd.RoomTiles.CreateIterator(); It; ++It)
	{
		ConnectedTiles.Add(It->Key, RoomIndex);
	}

	// Adding WallTiles to total ConnectedTiles
	for (auto It = RoomToAdd.WallTiles.CreateIterator(); It; ++It)
	{
		ConnectedTiles.Add(It->Key, RoomIndex);
	}

	// Adding CornerTiles to total ConnectedTiles
	for (auto It = RoomToAdd.CornerTiles.CreateIterator(); It; ++It)
	{
		ConnectedTiles.Add(*It, RoomIndex);
	}
}

// Generate nodes on map
void AMapGeneration::GenerateNodes()
{
	for (auto It = Rooms.CreateIterator(); It; ++It)
	{
		for (int X = 0; X < It->RoomSize.X; X++)
		{
			for (int Y = 0; Y < It->RoomSize.Y; Y++)
			{
				ANavigationNode* NewNode = GetWorld()->SpawnActor<ANavigationNode>(FVector(X + It->CornerTiles[0].X, Y + It->CornerTiles[0].Y, It->CornerTiles[0].Z) * Scale, FRotator::ZeroRotator, FActorSpawnParameters());
				It->RoomNodes.Add(NewNode);
			}
		}
	}
}

/*	Connect nodes room by room, and only connecting the W, NW, N and NE directions
	This will result in every node being connected in all 8 directions
	This also connects nodes through doors */
void AMapGeneration::ConnectNodes()
{
	// Iterate through rooms on the map
	for (auto It = Rooms.CreateIterator(); It; ++It)
	{
		// Iterate through every node in the room
		for (int X = 0; X < It->RoomSize.X; X++)
		{
			for (int Y = 0; Y < It->RoomSize.Y; Y++)
			{
				// Add W connection
				if (!(Y == 0))
				{
					AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], It->RoomNodes[(X * It->RoomSize.Y) + Y - 1]);
				}
				else
				{
					// Check what construction is to the West of the current tile
					FVector CurrentTile = FVector(X + It->CornerTiles[0].X, Y + It->CornerTiles[0].Y, It->CornerTiles[0].Z);
					ERoomConstructionType* Ptr = It->WallTiles.Find(CurrentTile);
					if (Ptr == nullptr)
					{
						if (It->CornerTiles.Contains(CurrentTile))
						{
							switch (It->CornerTiles.Find(CurrentTile))
							{
							case 0:
								Ptr = &(*(It->CornerTilesMap.Find(CurrentTile)))[1];
								break;
							case 1:
								Ptr = &(*(It->CornerTilesMap.Find(CurrentTile)))[0];
								break;
							}
						}
					}

					// If doorway is West of the current tile, connect it
					if (Ptr != nullptr)
					{
						if (*Ptr == ERoomConstructionType::DOORWAY)
						{
							// Find room to the West of the current tile
							int32* ConnectedRoomIndex = ConnectedTiles.Find(CurrentTile + FVector::LeftVector);
							Room* ConnectedRoom = &Rooms[*ConnectedRoomIndex];

							// Find index of connected tile in the connected room nodes
							int32 NodeX = (CurrentTile + FVector::LeftVector).X - ConnectedRoom->CornerTiles[0].X;
							int32 NodeY = (CurrentTile + FVector::LeftVector).Y - ConnectedRoom->CornerTiles[0].Y;

							AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], ConnectedRoom->RoomNodes[(NodeX * ConnectedRoom->RoomSize.Y) + NodeY]);
						}
					}
				}

				// Add NW connection
				if (!(X == It->RoomSize.X - 1 || Y == 0))
				{
					AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], It->RoomNodes[((X + 1) * It->RoomSize.Y) + Y - 1]);
				}
				
				// Add N connection
				if (!(X == It->RoomSize.X - 1))
				{
					AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], It->RoomNodes[((X + 1) * It->RoomSize.Y) + Y]);
				}
				else
				{
					// Check what construction is to the North of the current tile
					FVector CurrentTile = FVector(X + It->CornerTiles[0].X, Y + It->CornerTiles[0].Y, It->CornerTiles[0].Z);
					ERoomConstructionType* Ptr = It->WallTiles.Find(CurrentTile);
					if (Ptr == nullptr)
					{
						if (It->CornerTiles.Contains(CurrentTile))
						{
							switch (It->CornerTiles.Find(CurrentTile))
							{
							case 1:
								Ptr = &(*(It->CornerTilesMap.Find(CurrentTile)))[1];
								break;
							case 2:
								Ptr = &(*(It->CornerTilesMap.Find(CurrentTile)))[0];
								break;
							}
						}
					}

					// If doorway is North of the current tile, connect it
					if (Ptr != nullptr)
					{
						if (*Ptr == ERoomConstructionType::DOORWAY)
						{
							// Find room to the North of the current tile
							int32* ConnectedRoomIndex = ConnectedTiles.Find(CurrentTile + FVector::ForwardVector);
							Room* ConnectedRoom = &Rooms[*ConnectedRoomIndex];

							// Find index of connected tile in the connected room nodes
							int32 NodeX = (CurrentTile + FVector::ForwardVector).X - ConnectedRoom->CornerTiles[0].X;
							int32 NodeY = (CurrentTile + FVector::ForwardVector).Y - ConnectedRoom->CornerTiles[0].Y;

							AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], ConnectedRoom->RoomNodes[(NodeX * ConnectedRoom->RoomSize.Y) + NodeY]);
						}
					}
				}

				// Add NE connection
				if (!(X == It->RoomSize.X - 1 || Y == It->RoomSize.Y - 1))
				{
					AddConnection(It->RoomNodes[(X * It->RoomSize.Y) + Y], It->RoomNodes[((X + 1) * It->RoomSize.Y) + Y + 1]);
				}
			}
		}
	}
}

// Add connection between given nodes
void AMapGeneration::AddConnection(ANavigationNode* FromNode, ANavigationNode* ToNode)
{
	FVector DirectionVector = ToNode->GetActorLocation() - FromNode->GetActorLocation();
	DirectionVector.Normalize();

	if (!FromNode->ConnectedNodes.Contains(ToNode))
		FromNode->ConnectedNodes.Add(ToNode);

	if (!ToNode->ConnectedNodes.Contains(FromNode))
		ToNode->ConnectedNodes.Add(FromNode);
}

// Clears map for regeneration
void AMapGeneration::ClearMap()
{
	Rooms.Empty();
	ConnectedTiles.Empty();
	BlueSpawn.Empty();
	RedSpawn.Empty();

	for (TActorIterator<ARoomConstruction> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	bNoValidRooms = false;
}

// Spawns player starts and fills spawn nodes for AIManager to use when game starts
void AMapGeneration::SpawnTeams()
{
	// Find opposite spawn rooms
	TArray<Room*> BlueSpawnRooms;
	TArray<Room*> RedSpawnRooms;

	FindSpawnRooms(&BlueSpawnRooms, &RedSpawnRooms);

	// Select one random room
	Room* RoomToSpawnBlue = BlueSpawnRooms[FMath::RandRange(0, BlueSpawnRooms.Num() - 1)];
	Room* RoomToSpawnRed = RedSpawnRooms[FMath::RandRange(0, RedSpawnRooms.Num() - 1)];

	// Generate and randomize available spawn vectors for chosen room
	TArray<ANavigationNode*> SpawnNodesBlue = GenerateSpawnNodes(RoomToSpawnBlue);
	TArray<ANavigationNode*> SpawnNodesRed = GenerateSpawnNodes(RoomToSpawnRed);

	// Spawn PlayerStarts
	// Choose random door to look at when spawning
	FVector DoorToLookAtBlue = RandomDoorVector(RoomToSpawnBlue);
	FVector DoorToLookAtRed = RandomDoorVector(RoomToSpawnRed);

	APlayerStart* PlayerStartBlue = GetWorld()->SpawnActor<APlayerStart>(PlayerStartClass, SpawnNodesBlue[0]->GetActorLocation(), UKismetMathLibrary::FindLookAtRotation(SpawnNodesBlue[0]->GetActorLocation(), DoorToLookAtBlue));
	PlayerStartBlue->PlayerStartTag = "Blue";

	APlayerStart* PlayerStartRed = GetWorld()->SpawnActor<APlayerStart>(PlayerStartClass, SpawnNodesRed[0]->GetActorLocation(), UKismetMathLibrary::FindLookAtRotation(SpawnNodesRed[0]->GetActorLocation(), DoorToLookAtRed));
	PlayerStartRed->PlayerStartTag = "Red";

	// Add AI spawn points for AImanager
	for (int i = 1; i < SpawnNodesBlue.Num(); i++)
	{
		BlueSpawn.Add(SpawnNodesBlue[i]);
	}

	for (int i = 1; i < SpawnNodesRed.Num(); i++)
	{
		RedSpawn.Add(SpawnNodesRed[i]);
	}
}

// Generate and randomize available spawn vectors for chosen room
TArray<ANavigationNode*> AMapGeneration::GenerateSpawnNodes(Room* SpawnRoom)
{
	TArray<ANavigationNode*> SpawnNodes;

	UE_LOG(LogTemp, Warning, TEXT("RoomNodesNum: %d"), SpawnRoom->RoomNodes.Num());
	for (int i = 0; i < SpawnRoom->RoomNodes.Num(); i++)
	{
		SpawnNodes.Add(SpawnRoom->RoomNodes[i]);
	}

	for (int i = 0; i < SpawnNodes.Num(); i++)
	{
		int RandIndex = FMath::RandRange(0, SpawnNodes.Num() - 1);
		ANavigationNode* Temp = SpawnNodes[i];
		SpawnNodes[i] = SpawnNodes[RandIndex];
		SpawnNodes[RandIndex] = Temp;
	}
	UE_LOG(LogTemp, Warning, TEXT("SpawnNodes: %d"), SpawnNodes.Num());
	return SpawnNodes;
}

// Choose a random door in room to look at when spawning
FVector AMapGeneration::RandomDoorVector(Room* SpawnRoom)
{
	TArray<FVector*> Doors;

	for (auto It = SpawnRoom->CornerTilesMap.CreateIterator(); It; ++It)
	{
		if (It->Value[0] == ERoomConstructionType::DOORWAY || It->Value[1] == ERoomConstructionType::DOORWAY)
		{
			Doors.Add(&It->Key);
		}
	}

	for (auto It = SpawnRoom->WallTiles.CreateIterator(); It; ++It)
	{
		if (It->Value == ERoomConstructionType::DOORWAY)
		{
			Doors.Add(&It->Key);
		}
	}

	return *Doors[FMath::RandRange(0, Doors.Num()-1)];
}

// Find rooms to spawn teams
void AMapGeneration::FindSpawnRooms(TArray<Room*>* BlueSpawnRooms, TArray<Room*>* RedSpawnRooms)
{
	int32 MinX = 0;
	int32 MaxX = 0;
	int32 MinY = 0;
	int32 MaxY = 0;

	FindLimitsOfMap(&MinX, &MaxX, &MinY, &MaxY);

	UE_LOG(LogTemp, Warning, TEXT("MinX: %d"), MinX);
	UE_LOG(LogTemp, Warning, TEXT("MaxX: %d"), MaxX);
	UE_LOG(LogTemp, Warning, TEXT("MinY: %d"), MinY);
	UE_LOG(LogTemp, Warning, TEXT("MaxY: %d"), MaxY);

	if (MaxX - MinX >= MaxY - MinY)
	{
		for (auto It = ConnectedTiles.CreateIterator(); It; ++It)
		{
			if (It->Key.X == MinX)
			{
				if (!BlueSpawnRooms->Contains(&Rooms[It->Value]))
				{
					BlueSpawnRooms->Add(&Rooms[It->Value]);
				}
			}

			if (It->Key.X == MaxX)
			{
				if (!RedSpawnRooms->Contains(&Rooms[It->Value]))
				{
					RedSpawnRooms->Add(&Rooms[It->Value]);
				}
			}
		}
	}
	else
	{
		for (auto It = ConnectedTiles.CreateIterator(); It; ++It)
		{
			if (It->Key.Y == MinY)
			{
				if (!BlueSpawnRooms->Contains(&Rooms[It->Value]))
				{
					BlueSpawnRooms->Add(&Rooms[It->Value]);
				}
			}

			if (It->Key.Y == MaxY)
			{
				if (!RedSpawnRooms->Contains(&Rooms[It->Value]))
				{
					RedSpawnRooms->Add(&Rooms[It->Value]);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("1BlueRoomNum: %d"), BlueSpawnRooms->Num());
	UE_LOG(LogTemp, Warning, TEXT("1RedRoomNum: %d"), RedSpawnRooms->Num());
}

// Find X and Y limits of the generated map
void AMapGeneration::FindLimitsOfMap(int32* MinX, int32* MaxX, int32* MinY, int32* MaxY)
{
	for (auto It = ConnectedTiles.CreateIterator(); It; ++It)
	{
		*MinX = (It->Key.X < *MinX) ? It->Key.X : *MinX;
		*MaxX = (It->Key.X > *MaxX) ? It->Key.X : *MaxX;
		*MinY = (It->Key.Y < *MinY) ? It->Key.Y : *MinY;
		*MaxY = (It->Key.Y > *MaxY) ? It->Key.Y : *MaxY;
	}
}

