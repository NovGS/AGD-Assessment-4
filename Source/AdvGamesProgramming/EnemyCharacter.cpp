
// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "AIManager.h"
#include "NavigationNode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Math/UnrealMathUtility.h"
#include "HealthComponent.h"



// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CurrentAgentState = AgentState::PATROL;
	PathfindingNodeAccuracy = 100.0f;
	SprintMultiplier = 1.5f;

	//Set AI's GenericTeamID to 0, In the EnemyCharacterBlueprint set the AI perception to only detect Enemy
	TeamId = FGenericTeamId(0);
	//bDoOnce = false;
	//bCanHeal = false;
	
}


// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	Cast<UCharacterMovementComponent>(GetMovementComponent())->bOrientRotationToMovement = true;

	HealthComponent = FindComponentByClass<UHealthComponent>();


	PerceptionComponent = FindComponentByClass<UAIPerceptionComponent>();
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::SensePlayer);
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::SenseHealthPickUp);
	}

	DetectedPlayer = nullptr;
	bCanSeePlayer = false;
	DetectedHealthPickup = nullptr;
	bCanSeeHealthPickup = false;


}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HealthComponent->CurrentHealth > 0)
	{
		if (CurrentAgentState == AgentState::PATROL)
		{
			AgentPatrol();
			if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{

				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}

			else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}

			else if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeeHealthPickup)
			{
				CurrentAgentState = AgentState::HEAL;
				Path.Empty();
			}
		}

		else if (CurrentAgentState == AgentState::ENGAGE)
		{
			AgentEngage();
			if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{
				CurrentAgentState = AgentState::PATROL;
			}

			else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}
		}

		else if (CurrentAgentState == AgentState::EVADE)
		{
			AgentEvade();
			if (bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::HEAL;
				Path.Empty();
			}

			else if (!bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f && !bCanSeePlayer)
			{
				CurrentAgentState = AgentState::PATROL;
				Path.Empty();
			}

			else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{
				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}
		}

		else if (CurrentAgentState == AgentState::HEAL)
		{
			AgentHeal();

			//If AI get the healthpickup and can see an Enemy. Then Re-Engage
			if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{
				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}

			//If AI get the healthpickup and can not see an Enemy. Then Patrol (search for player)
			else if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{
				CurrentAgentState = AgentState::PATROL;
				Path.Empty();
			}

			//If AI didnt get the healthpickup, and can see an Enemy. Then Evade
			else if (!bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeePlayer)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}

			//If AI didnt get the healthpickup, and can not see an Enemy. Then Patrol (search for healthpickup)
			else if (!bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f && !bCanSeePlayer)
			{
				CurrentAgentState = AgentState::PATROL;
				Path.Empty();
			}

				

		}
		/*
		else if (CurrentAgentState == AgentState::CHASE)
		{
			AgentChase();
			if (bCanSeePlayer)
			{
				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}

		
			else if (!bCanSeePlayer)
			{
				CurrentAgentState = AgentState::PATROL;
			}
	

			else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}
		}
		*/

        /*
		else if (CurrentAgentState == AgentState::CHECK)
		{
			AgentCheck();

			if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}

			if (!bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f && bCanHeal)
			{
				CurrentAgentState = AgentState::HEAL;
			}	
		}

		else if (CurrentAgentState == AgentState::HEAL)
		{
			AgentHeal();

			if (HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeeActor)
			{
				bCanHeal = false;
				CurrentAgentState = AgentState::EVADE;
			}

			else if (HealthComponent->HealthPercentageRemaining() >= 40.0f && bCanSeeActor)
			{
				bCanHeal = false;
				CurrentAgentState = AgentState::ENGAGE;
			}

			else if (HealthComponent->HealthPercentageRemaining() >= 40.0f && !bCanSeeActor)
			{
				bCanHeal = false;
				CurrentAgentState = AgentState::PATROL;
			}
		}
		*/
		MoveAlongPath();
		//Reload();
	}

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::AgentPatrol()
{
	if (Path.Num() == 0)
	{
		if (Manager)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enter Patrol"));
			Path = Manager->GeneratePath(CurrentNode, Manager->AllNodes[FMath::RandRange(0, Manager->AllNodes.Num() - 1)]);
		}
	}
}

void AEnemyCharacter::AgentEngage()
{
	if (bCanSeePlayer && DetectedPlayer)
	{
		FVector FireDirection = DetectedPlayer->GetActorLocation() - GetActorLocation();
		Fire(FireDirection);
	}
	if (Path.Num() == 0 && DetectedPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter Engage"));
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedPlayer->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

void AEnemyCharacter::AgentEvade()
{
	if (DetectedPlayer && bCanSeePlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter Evade"));
		ANavigationNode* FurthestNode = Manager->FindFurthestNode(DetectedPlayer->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, FurthestNode);
	}
}

void AEnemyCharacter::AgentHeal()
{
	if (DetectedHealthPickup && bCanSeeHealthPickup && Path.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter Heal"));
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedHealthPickup->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

void AEnemyCharacter::AgentChase()
{
    //Once the AI cant see the player, it will record the player's last seen location and chase to that location.
	
		ANavigationNode* NearestNode = Manager->FindNearestNode(PlayerLocation);
		Path = Manager->GeneratePath(CurrentNode, NearestNode);

}

/*
void AEnemyCharacter::AgentCheck()
{

		FRotator Current = GetActorRotation();

		float value = 180.0f;

		FRotator Target = Current;
		Target.Yaw += value;
		SetActorRotation(FMath::RInterpTo(Current, Target, GetWorld()->GetDeltaSeconds(), 2.0f));

		if (DoOnce)
		{
			DoOnce = false;
			GetWorldTimerManager().SetTimer(SpinTimerHandle, this, &AEnemyCharacter::SetCanHealToTrue, 1.0f, false, 3.0f);
		}
		
		if (bCanSeeActor)
		{
			GetWorldTimerManager().ClearTimer(SpinTimerHandle);
		}

}

*/
/*
void AEnemyCharacter::AgentHeal()
{
	HealthComponent->CurrentHealth += GetWorld()->GetDeltaSeconds() * 2;
	GetWorldTimerManager().ClearTimer(SpinTimerHandle);
}

*/

//As the player's GenericTeamId is set to 1, which is considered as the enemy of AI. 
//Thus, the AI will react to the player.
void AEnemyCharacter::SensePlayer(AActor* ActorSensed, FAIStimulus Stimulus)
{
	//Cast to the IGenericTeamAgentInterface using ActorSensed (Player) as parameter
		if (IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(ActorSensed))
		{
	        //When sensed Player
			if (TeamAgentInterface->GetGenericTeamId() == 1)
			{
				if (Stimulus.WasSuccessfullySensed())
				{
					//UE_LOG(LogTemp, Warning, TEXT("Player Detected"));
					DetectedPlayer = ActorSensed;
					PlayerLocation = ActorSensed->GetActorLocation();
					bCanSeePlayer = true;
				}
				else
				{
					//UE_LOG(LogTemp, Warning, TEXT("Player Lost"));
					bCanSeePlayer = false;
				}
			}
		}
}

void AEnemyCharacter::SenseHealthPickUp(AActor* ActorSensed, FAIStimulus Stimulus)
{
	if (IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(ActorSensed))
	{
		//When sensed Player
		if (TeamAgentInterface->GetGenericTeamId() == 2)
		{
			if (Stimulus.WasSuccessfullySensed())
			{
				//UE_LOG(LogTemp, Warning, TEXT("Health Pickup Detected"));
				DetectedHealthPickup = ActorSensed;
				bCanSeeHealthPickup = true;
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("Health pickup lost"));
				bCanSeeHealthPickup = false;
			}
		}
	}
}

void AEnemyCharacter::Reload()
{
	BlueprintReload();
}


/*
void AEnemyCharacter::SetCanHealToTrue()
{
	bCanHeal = true;
}
*/


void AEnemyCharacter::MoveAlongPath()
{
	if ((GetActorLocation() - CurrentNode->GetActorLocation()).IsNearlyZero(PathfindingNodeAccuracy)
		&& Path.Num() > 0)
	{
		CurrentNode = Path.Pop();
	}
	else if (!(GetActorLocation() - CurrentNode->GetActorLocation()).IsNearlyZero(PathfindingNodeAccuracy))
	{
		AddMovementInput(CurrentNode->GetActorLocation() - GetActorLocation());
	}
}


FGenericTeamId AEnemyCharacter::GetGenericTeamId() const
{
	return TeamId;
}







