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
	HEAL,
	RELOAD
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

	TArray <class ANavigationNode* > Path;
	ANavigationNode* CurrentNode;

	UPROPERTY(VisibleAnywhere)
	class AAIManager* Manager;

	UPROPERTY(EditAnywhere, meta=(UIMin="10.0", UIMax="1000.0", ClampMin="10.0", ClampMax="1000.0"))
	float PathfindingNodeAccuracy;

	//Similar to PathfindingNodeAccuracy, but for pickup
	UPROPERTY(EditAnywhere, meta = (UIMin = "10.0", UIMax = "1000.0", ClampMin = "10.0", ClampMax = "1000.0"))
	float PickupAccuracy;

	class UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	AgentState CurrentAgentState;

	class UAIPerceptionComponent* PerceptionComponent;

	//Detected Player include all Enemy from opposite team.
	AActor* DetectedPlayer;
	//bCanSeePlyaer include all Enemy from opposite team
	bool bCanSeePlayer;

	AActor* DetectedHealthPickup;
	bool bCanSeeHealthPickup;

	AActor* DetectedWeaponPickup;
	bool bCanSeeWeaponPickup;

	//True:  No bullet left
	//False: Have bullet left 
	UPROPERTY(BlueprintReadWrite)
	bool bIsBulletEmpty;


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void AgentPatrol();
	void AgentEngage();
	void AgentEvade();
	void AgentHeal();
	void AgentReload();

	UFUNCTION(blueprintCallable)
	void SensePlayer(AActor* ActorSensed, FAIStimulus Stimulus);

	UFUNCTION(blueprintCallable)
	void SenseHealthPickup(AActor* ActorSensed, FAIStimulus Stimulus);

	UFUNCTION(blueprintCallable)
	void SenseWeaponPickup(AActor* ActorSensed, FAIStimulus Stimulus);

	UFUNCTION(BlueprintCallable, blueprintimplementableevent)
	void Fire(FVector FireDirection);

	//Team Id for the AI
	FGenericTeamId TeamId;

private:

	void MoveAlongPath();

	virtual FGenericTeamId GetGenericTeamId() const override;

	// Called when Material variable gets changed
	UFUNCTION()
	virtual void OnRep_SetMaterial();

	// Replicated to allow AI mesh to change colours according to their TeamID
	UPROPERTY(ReplicatedUsing = OnRep_SetMaterial)
	UMaterialInstance* Material;

	// Materials to change to
	UMaterialInstance* RedMaterial;
	UMaterialInstance* BlueMaterial;
};
