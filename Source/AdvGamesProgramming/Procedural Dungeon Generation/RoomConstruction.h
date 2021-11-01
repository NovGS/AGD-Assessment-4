// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomConstruction.generated.h"

UENUM(BlueprintType)
enum class ERoomConstructionType : uint8
{
	WALL,
	DOORWAY,
	TILE
};

UCLASS()
class ADVGAMESPROGRAMMING_API ARoomConstruction : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomConstruction();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
