
// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyCharacter.h"
#include "AIManager.h"
#include "NavigationNode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Math/UnrealMathUtility.h"
#include "HealthComponent.h"
#include "Materials/MaterialInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CurrentAgentState = AgentState::PATROL;
	PathfindingNodeAccuracy = 100.0f;
	PickupAccuracy = 50.0f;
	SprintMultiplier = 1.5f;

	// Retrieve Red and Blue materials for the AI Mesh material
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceBlueObject(TEXT("/Game/Assets/Mannequin/UE4_Mannequin/Materials/M_UE4Man_Body_Blue"));
	BlueMaterial = MaterialInstanceBlueObject.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceRedObject(TEXT("/Game/Assets/Mannequin/UE4_Mannequin/Materials/M_UE4Man_Body_Red"));
	RedMaterial = MaterialInstanceRedObject.Object;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set AI Mesh material to be Blue or Red depending on their team ID
	// This gets replicated in OnRep_SetMaterial()
	switch (TeamId)
	{
	case 0:
		Material = BlueMaterial;
		break;
	case 1:
		Material = RedMaterial;
		break;
	}

	Cast<UCharacterMovementComponent>(GetMovementComponent())->bOrientRotationToMovement = true;

	HealthComponent = FindComponentByClass<UHealthComponent>();

	PerceptionComponent = FindComponentByClass<UAIPerceptionComponent>();
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::SensePlayer);
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::SenseHealthPickup);
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::SenseWeaponPickup);
	}

	DetectedPlayer = nullptr;
	bCanSeePlayer = false;
	DetectedHealthPickup = nullptr;
	bCanSeeHealthPickup = false;
	DetectedWeaponPickup = nullptr;
	bCanSeeWeaponPickup = false;

	bIsBulletEmpty = false;
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
			//AI will search for different things in this state, depending on its health and bullet left. AI's target can be Player, Health pickup or Weapon Pickup.
			if (CurrentAgentState == AgentState::PATROL)
			{
				AgentPatrol();

				//if AI can see an Enemy, the health is above 40%, and Still have bullet. Then engage with enemy
				if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f && !bIsBulletEmpty )
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

				//if AI dont have bullet left, and health is above 40%, Then Reload
				else if (bIsBulletEmpty && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::RELOAD;
					//Abandon current path, get the weapon pickup immediately
					Path.Empty();
				}
			}

			//In Engage State
			else if (CurrentAgentState == AgentState::ENGAGE)
			{
				AgentEngage();

				//if AI can not see an Enemy, and the health is above 40%. Then Partrol (Search for Enemy)
				if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f && !bIsBulletEmpty)
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

				//if AI run out of bullet in the engage state,
				else if (bIsBulletEmpty)
				{
					CurrentAgentState = AgentState::RELOAD;
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

				//If health is above 40% and can see an Enemy. Then Re-Engage
				if (bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::ENGAGE;
					//Abandon current path, engage immediately
					Path.Empty();
				}

				//If health is above 40% and can not see an Enemy. Then Patrol (search for Enemy)
				else if (!bCanSeePlayer && HealthComponent->HealthPercentageRemaining() >= 40.0f)
				{
					CurrentAgentState = AgentState::PATROL;
					//Abandon current path
					Path.Empty();
				}

				//If can not see a health pickup (health pickup is taken by someone else), health is below 40% and can see an Enemy. Then Evade
				else if (!bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeePlayer)
				{
					CurrentAgentState = AgentState::EVADE;
					//Abandon current path, evade immediately
					Path.Empty();
				}

				//If AI can not see a health pickup (health pickup is taken by someone else), health is below 40%, and can not see an Enemy. Then Patrol (search for healthpickup)
				else if (!bCanSeeHealthPickup && HealthComponent->HealthPercentageRemaining() < 40.0f && !bCanSeePlayer)
				{
					CurrentAgentState = AgentState::PATROL;
					//Abandon current path
					Path.Empty();
				}
			}

			//In Reload State
			else if (CurrentAgentState == AgentState::RELOAD)
			{
				AgentReload();

				// if AI get the weapon pickup, health is above 40% and can see an enemy. then re-engage
				if (!bIsBulletEmpty && HealthComponent->HealthPercentageRemaining() >= 40.0f && bCanSeePlayer)
				{
					CurrentAgentState = AgentState::ENGAGE;
					//abandon current path, engage immediately
					Path.Empty();
				}

				// if AI get the weapon pickup, health is above 40% and can not see an enemy. then patrol (search for enemy)
				else if (!bIsBulletEmpty && HealthComponent->HealthPercentageRemaining() >= 40.0f && !bCanSeePlayer)
				{
					CurrentAgentState = AgentState::PATROL;
					//abandon current path
					Path.Empty();
				}

				// if AI's health is below 40%  and can see an enemy. then evade (situation like ai got damaged during the movement)
				else if (HealthComponent->HealthPercentageRemaining() < 40.0f && bCanSeePlayer)
				{
					CurrentAgentState = AgentState::EVADE;
					Path.Empty();
				}

				// if AI's health is below 40% and can not see an enemy. then patrol (situation like ai got damage from the back, and can not see an enemy)
				else if (HealthComponent->HealthPercentageRemaining() < 40.0f && !bCanSeePlayer)
				{
					CurrentAgentState = AgentState::PATROL;
					Path.Empty();
				}

			}
			MoveAlongPath();
		}

	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// Sets Material as replicated
void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyCharacter, Material);
}

void AEnemyCharacter::AgentPatrol()
{
	if (Path.Num() == 0)
	{
		if (Manager)
		{
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
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedPlayer->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

void AEnemyCharacter::AgentEvade()
{
	if (DetectedPlayer && bCanSeePlayer)
	{
		ANavigationNode* FurthestNode = Manager->FindFurthestNode(DetectedPlayer->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, FurthestNode);
	}
}

void AEnemyCharacter::AgentHeal()
{
	if (DetectedHealthPickup && bCanSeeHealthPickup)
	{
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedHealthPickup->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

void AEnemyCharacter::AgentReload()
{
	if (!bCanSeeWeaponPickup)
	{
		Path = Manager->GeneratePath(CurrentNode, Manager->AllNodes[FMath::RandRange(0, Manager->AllNodes.Num() - 1)]);
	}

	if (DetectedWeaponPickup && bCanSeeWeaponPickup)
	{
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedWeaponPickup->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

//As the player's GenericTeamId is set to 1, which is considered as the enemy of AI.
//Thus, the AI will react to the player.
void AEnemyCharacter::SensePlayer(AActor* ActorSensed, FAIStimulus Stimulus)
{
	//Cast to the IGenericTeamAgentInterface using ActorSensed (Player) as parameter
		if (IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(ActorSensed))
		{
			// Sets EnemyID to be the opposite of the TeamID, 0 or 1
			int32 EnemyId = (TeamId == 0) ? 1 : 0;

	        //When sensed Player
			if (TeamAgentInterface->GetGenericTeamId() == EnemyId)
			{
				if (Stimulus.WasSuccessfullySensed())
				{
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

//The health pickup's GenericTeamId is set to 2, which is the enemy of AI.
//Thus, the AI will react to the health pickup, different to player
void AEnemyCharacter::SenseHealthPickup(AActor* ActorSensed, FAIStimulus Stimulus)
{
	if (IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(ActorSensed))
	{
		//When sensed actor's TeamId = 2, indicates thats a health pickup
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

//The Weapon Pickup's GenericTeamId is set to 3
void AEnemyCharacter::SenseWeaponPickup(AActor* ActorSensed, FAIStimulus Stimulus)
{
	if (IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(ActorSensed))
	{
		//When sensed actor's TeamId = 3, indicates thats a Weapon pickup
		if (TeamAgentInterface->GetGenericTeamId() == 3)
		{
			if (Stimulus.WasSuccessfullySensed())
			{
				DetectedWeaponPickup = ActorSensed;
				bCanSeeWeaponPickup = true;
			}
			else
			{
				bCanSeeWeaponPickup = false;
			}
		}
	}
}

void AEnemyCharacter::MoveAlongPath()
{
	if ((GetActorLocation() - CurrentNode->GetActorLocation()).IsNearlyZero(PathfindingNodeAccuracy) && Path.Num() > 0)
	{
		CurrentNode = Path.Pop();
	}
	else if (!(GetActorLocation() - CurrentNode->GetActorLocation()).IsNearlyZero(PickupAccuracy))
	{
		AddMovementInput(CurrentNode->GetActorLocation() - GetActorLocation());
	}
}

// Sets AI mesh material when Material variable is updated
void AEnemyCharacter::OnRep_SetMaterial()
{
	USkeletalMeshComponent* SkeletalMesh = FindComponentByClass<USkeletalMeshComponent>();
	SkeletalMesh->SetMaterial(0, Material);
}


FGenericTeamId AEnemyCharacter::GetGenericTeamId() const
{
	return TeamId;
}
