
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

	DoOnce = false;
	bCanHeal = false;

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
	}

	DetectedActor = nullptr;
	bCanSeeActor = false;

	USkeletalMeshComponent* mesh = GetMesh();

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
			if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{
				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}
			else if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}
		}
		else if (CurrentAgentState == AgentState::ENGAGE)
		{
			AgentEngage();
			if (!bCanSeeActor)
			{
				CurrentAgentState = AgentState::CHASE;
				Path.Empty();
			}
			else if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}
		}

		else if (CurrentAgentState == AgentState::EVADE)
		{
			AgentEvade();

			if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() >= 40.0f)
			{

				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}

			else if (!bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				DoOnce = true;
				CurrentAgentState = AgentState::CHECK;
			}
		}

		else if (CurrentAgentState == AgentState::CHASE)
		{
			AgentChase();
			if (bCanSeeActor)
			{
				CurrentAgentState = AgentState::ENGAGE;
				Path.Empty();
			}

		
			else if (!bCanSeeActor)
			{
				CurrentAgentState = AgentState::PATROL;
			}
	

			else if (bCanSeeActor && HealthComponent->HealthPercentageRemaining() < 40.0f)
			{
				CurrentAgentState = AgentState::EVADE;
				Path.Empty();
			}
			
		}

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
			Path = Manager->GeneratePath(CurrentNode, Manager->AllNodes[FMath::RandRange(0, Manager->AllNodes.Num() - 1)]);
		}
	}
}

void AEnemyCharacter::AgentEngage()
{
	if (bCanSeeActor && DetectedActor)
	{
		FVector FireDirection = DetectedActor->GetActorLocation() - GetActorLocation();
		Fire(FireDirection);
	}
	if (Path.Num() == 0 && DetectedActor)
	{
		ANavigationNode* NearestNode = Manager->FindNearestNode(DetectedActor->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, NearestNode);
	}
}

void AEnemyCharacter::AgentEvade()
{
	//Make the AI run faster on Evade state, it looks like the AI is trying to run away from the danger to avoid death.
	//Which i think make it more realistic.
	if (bCanSeeActor && DetectedActor)
	{
		GetCharacterMovement()->MaxWalkSpeed *= SprintMultiplier;
	}

	//The movement speed will back to normal when the AI can't see the player
	if (!bCanSeeActor)
	{
		GetCharacterMovement()->MaxWalkSpeed /= SprintMultiplier;
	}


	if (Path.Num() == 0 && DetectedActor)
	{
		ANavigationNode* FurthestNode = Manager->FindFurthestNode(DetectedActor->GetActorLocation());
		Path = Manager->GeneratePath(CurrentNode, FurthestNode);
	}
}

void AEnemyCharacter::AgentChase()
{
    //Once the AI cant see the player, it will record the player's last seen location and chase to that location.
	
		ANavigationNode* NearestNode = Manager->FindNearestNode(PlayerLocation);
		Path = Manager->GeneratePath(CurrentNode, NearestNode);

}

void AEnemyCharacter::AgentCheck()
{

		FRotator Current = GetActorRotation();

		//Check to see if the location is safe. if it is not safe the AI will go back to Evade state. if it is safe, it will go to heal state
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

void AEnemyCharacter::AgentHeal()
{
	HealthComponent->CurrentHealth += GetWorld()->GetDeltaSeconds() * 2;
	GetWorldTimerManager().ClearTimer(SpinTimerHandle);
}



void AEnemyCharacter::SensePlayer(AActor* ActorSensed, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Warning, TEXT("Player Detected"))
		DetectedActor = ActorSensed;
		PlayerLocation = ActorSensed->GetActorLocation();
		bCanSeeActor = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player Lost"))
		bCanSeeActor = false;
	}
}

void AEnemyCharacter::Reload()
{
	BlueprintReload();
}

void AEnemyCharacter::SetCanHealToTrue()
{
	bCanHeal = true;
}



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



