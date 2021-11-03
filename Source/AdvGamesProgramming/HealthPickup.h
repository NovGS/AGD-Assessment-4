// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GenericTeamAgentInterface.h"
#include "HealthPickup.generated.h"

/**
 * 
 */
UCLASS()
class ADVGAMESPROGRAMMING_API AHealthPickup : public APickup, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AHealthPickup();
	void OnPickup(AActor* ActorThatPickedUp) override;

private:
	FGenericTeamId TeamId;
	virtual FGenericTeamId GetGenericTeamId() const override;
	
};
