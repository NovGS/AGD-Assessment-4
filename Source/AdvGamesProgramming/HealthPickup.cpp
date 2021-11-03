// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "GameFramework/Actor.h"
#include "HealthComponent.h"

//Set Default values
AHealthPickup::AHealthPickup()
{
	TeamId = FGenericTeamId(2);
}


void AHealthPickup::OnPickup(AActor* ActorThatPickedUp)
{
	Super::OnPickup(ActorThatPickedUp);

	UHealthComponent* Health = ActorThatPickedUp->FindComponentByClass<UHealthComponent>();
	if (Health)
	{
		Health->CurrentHealth = 100;
		Destroy();
	}

	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to find health component"));
	}

}

FGenericTeamId AHealthPickup::GetGenericTeamId() const
{
	return TeamId;
}

