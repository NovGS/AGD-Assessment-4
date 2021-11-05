
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

	CurrentAgentState = AgentState::PATROL;
	AgentPatrol();


}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Manager)
	{
		//AI behavior only work when the health is above 0
		if (HealthComponent->CurrentHealth > 0)
		{
			//In Patrol State
			//AI will search for different things in this state, depending on its health. AI can either looking for Enemy or Health pickup.
			if (CurrentAgentState == AgentState::PATROL)
			{
				AgentPatrol();

				//if AI can see an Enemy, and the health is above 40%. Then engage with enemy
				if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::ENGAGE;
					//Abandon current path, engage immediately 
					Path.Empty();
				}

				//if AI can see an Enemy, and the health is below 40%. Then Evade the enemy
				else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f)
				{
					CurrentAgentState = AgentState::EVADE;
					//Abandon current path, evade immediately
					Path.Empty();
				}

				//if AI can not see an Enemy, and health is below 40%, and can see a health pickup. Then go pickup that health pickup
				else if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeeHealthPickup)
				{
					CurrentAgentState = AgentState::HEAL;
					//Abandon current path, get the health pickup immediately
					Path.Empty();
				}
			}

			//In Engage State
			else if (CurrentAgentState == AgentState::ENGAGE)
			{
				AgentEngage();

				//if AI can not see an Enemy, and the health is above 40%. Then Partrol (Search for Enemy)
				if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::PATROL;
					//Do not abandon current path, if the AI lost the enemy in sight, follow the path to the end to search for enemy
				}

				//if AI can see an Enemy, and the health is below 40%. Then Evade the enemy
				else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() < 40.0f)
				{
					CurrentAgentState = AgentState::EVADE;
					//Abandon current path, quit engage and evade immediately
					Path.Empty();
				}
			}

			//In Evade State
			//In this state, AI will get the health pickup priorly when both Enemy and health pickup are visible. 
			else if (CurrentAgentState == AgentState::EVADE)
			{
				AgentEvade();

				//if AI can see a health pickup and health is below 40%. Then go pickup that health pickup
				if (bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f)
				{
					CurrentAgentState = AgentState::HEAL;
					//Abandon current path, get the health pickup immediately. (Even with an Enemy nearby)
					Path.Empty();
				}

				//if AI can not see an Enemy, can not see a health pickup, and health is below 40%. Then Patrol (search for health pickup)
				else if (!bCanSeePlayer && !bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f)
				{
					CurrentAgentState = AgentState::PATROL;
					//Do not abandon current path, evade to a safe position first before go back to patrol (search for health pickup)
				}

				//if AI can see an Enemy, and health is above 40%. Then Engage with the Enemy. (This only occurs when AI didn't see the health pickup but accidentally stepped on the health pickup during the process of Evading)
				else if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::ENGAGE;
					//Abandon current path, engage immediately
					Path.Empty();
				}
			}

			//In Heal State
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
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::AgentPatrol()
{
	UE_LOG(LogTemp, Warning, TEXT("Path Num in Patrol is : %d"), Path.Num());
	if (Path.Num() == 0)
	{
		if (Manager)
		{
			UE_LOG(LogTemp, Warning, TEXT("Manager was found in Patrol"));
			UE_LOG(LogTemp, Warning, TEXT("Enter Patrol"));
			Path = Manager->GeneratePath(CurrentNode, Manager->AllNodes[FMath::RandRange(0, Manager->AllNodes.Num() - 1)]);
			UE_LOG(LogTemp, Warning, TEXT("Enter Patrol1"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No manager found in Patrol"));
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
	
	if ((GetActorLocation() - CurrentNode->GetActorLocation()).IsNearlyZero(PathfindingNodeAccuracy) && Path.Num() > 0)
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







