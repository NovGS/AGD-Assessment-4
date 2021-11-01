// Fill out your copyright notice in the Description page of Project Settings.


#include "Room.h"

Room::Room()
{


}

// Clears all room variables
void Room::ClearRoom()
{
	RoomTiles.Empty();
	WallTiles.Empty();
	CornerTiles.Empty();
	CornerTilesMap.Empty();

	AvailableWallTiles.Empty();
	RoomSize = FVector2D::ZeroVector;
}

