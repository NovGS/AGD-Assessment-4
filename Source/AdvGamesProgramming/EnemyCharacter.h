// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "EnemyCharacter.generated.h"

UENUM()
enum class AgentState : uint8
{
	PATROL,
	ENGAGE,
	EVADE,
	CHASE,
	HEAL
	//CHECK,
	//HEAL
};

UCLASS()
class ADVGAMESPROGRAMMING_API AEnemyCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	//FTimerHandle SpinTimerHandle;
	//bool bDoOnce;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	//bool bCanHeal;
	TArray <class ANavigationNode* > Path;
	ANavigationNode* CurrentNode;
	
	UPROPERTY(VisibleAnywhere)
	class AAIManager* Manager;

	UPROPERTY(EditAnywhere, meta=(UIMin="10.0", UIMax="1000.0", ClampMin="10.0", ClampMax="1000.0"))
	float PathfindingNodeAccuracy;

	class UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	AgentState CurrentAgentState;

	class UAIPerceptionComponent* PerceptionComponent;

	AActor* DetectedPlayer;
	bool bCanSeePlayer;
	AActor* DetectedHealthPickup;
	bool bCanSeeHealthPickup;

	FVector PlayerLocation;
	float SprintMultiplier;


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void AgentPatrol();
	void AgentEngage();
	void AgentEvade();
	void AgentChase();
	void AgentHeal();
	//void AgentCheck();
	//void AgentHeal();

	UFUNCTION(blueprintCallable)
	void SensePlayer(AActor* ActorSensed, FAIStimulus Stimulus);

	UFUNCTION(blueprintCallable)
	void SenseHealthPickUp(AActor* ActorSensed, FAIStimulus Stimulus);

	UFUNCTION(BlueprintImplementableEvent)
	void Fire(FVector FireDirection);


	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintReload();

	void Reload();


	//void SetCanHealToTrue();

private:

	void MoveAlongPath();

	//Set up team id for the AI
	FGenericTeamId TeamId;
	virtual FGenericTeamId GetGenericTeamId() const override;
};
