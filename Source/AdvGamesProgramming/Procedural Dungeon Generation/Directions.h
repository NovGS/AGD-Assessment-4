// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "Directions.generated.h"

/**
 * 
 */

UENUM()
enum class EDirections : uint8
{
	N,
	E,
	S,
	W
};

UCLASS()
class ADVGAMESPROGRAMMING_API UDirections : public UUserDefinedEnum
{
	GENERATED_BODY()
	
};
