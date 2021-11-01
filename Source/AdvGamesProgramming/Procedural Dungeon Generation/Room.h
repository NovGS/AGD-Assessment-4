// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomConstruction.h"

/**
 * 
 */
class ADVGAMESPROGRAMMING_API Room
{
public:
	Room();
	
	// Map of Roomtiles which are not Wall or CornerTiles
	TMap<FVector, ERoomConstructionType> RoomTiles;

	// Map of WallTiles
	TMap<FVector, ERoomConstructionType> WallTiles;

	// Array of CornerTiles clockwise from BottomLeft, TopLeft, TopRight and BottomRight
	TArray<FVector> CornerTiles;
	// Map of CornerTiles which contains information about whether that tile is a wall or doorway
	// Array is also clockwise so BottomLeft corner, [0] is Southern border and [1] is Western border
	TMap<FVector, TArray<ERoomConstructionType>> CornerTilesMap;

	// All tiles that are valid doorspaces
	TArray<FVector> AvailableWallTiles;

	// Size of the room FVector2D(X, Y)
	FVector2D RoomSize;

	void ClearRoom();
};
