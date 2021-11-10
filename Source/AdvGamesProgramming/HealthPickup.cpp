// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "GameFramework/Actor.h"
#include "HealthComponent.h"
#include "Components/BoxComponent.h"

//Set Default values
AHealthPickup::AHealthPickup()
{
	//Set TeamId to 2
	TeamId = FGenericTeamId(2);

	// Set boundingBo size to better suit the health pickup
	PickupBoundingBox->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
}


void AHealthPickup::OnPickup(AActor* ActorThatPickedUp)
{
	Super::OnPickup(ActorThatPickedUp);

	//Destory the object and rest the health of ActorThatPickedUp to 100
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

