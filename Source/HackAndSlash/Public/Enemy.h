
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"

enum EDamageTypes : uint8;
class UTimelineComponent;

UCLASS()
class HACKANDSLASH_API AEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemy();

	void PlayHitReact(TEnumAsByte<EDamageTypes> DamageType, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable)
	void ResetEnemy();

	virtual void Landed(const FHitResult& Hit) override;

	void EnemyFinisher(AActor* Player);
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UClass*> CanLandedClasses;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* HR_Left;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* HR_Right;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* HR_Middle;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* HR_KnockBack;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* HR_AirKnockBack;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* AirHit;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* Falling;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* K_Landed;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* AirKnockBackLanded;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* KnockDownCrash;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* EnemyExecution;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	FVector DesiredLaunchLocation;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	FVector EnemyGroundLocation;

	bool bAirKnockBack;

	bool bAirAttacked;

	bool bDead;


protected:
	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
		AActor* DamageCauser) override;

	
protected:
	
	void Buffer(float Buffer);
	float BufferAmount;
	void StopBuffer();
	UFUNCTION()
	void BufferTimelineTickCallBack();
	UPROPERTY()
	UTimelineComponent* BufferTimeline;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	UCurveFloat* BufferEnemyCurve;

	UFUNCTION(BlueprintImplementableEvent)
	void Launch();
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void StopLaunch();
	
	UFUNCTION(BlueprintImplementableEvent)
	void KnockDownCrasher();

};
