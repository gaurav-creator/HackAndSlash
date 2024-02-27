
#pragma once

#include "CoreMinimal.h"
#include "DoOnce.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "SlashCharacter.generated.h"

class AEnemy;
enum EStates : uint8;
class UTimelineComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class HACKANDSLASH_API ASlashCharacter : public ACharacter
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	ASlashCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);
	

public:	
	virtual void Tick(float DeltaTime) override;
	
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	UStaticMeshComponent* Weapon;

	void SetState(TEnumAsByte<EStates> NewState);
	TEnumAsByte<EStates> GetState() const;
	bool IsStateEqualToAny(const TArray<TEnumAsByte<EStates>>& StateToCheck) const;
	
	void LightAttackPressed();
	void LightAttackReleased();
	void LightAttackEvent();
	UFUNCTION(BlueprintCallable)
	void SaveLightAttack();
	
	void HeavyAttack();
	void HeavyAttackEvent();
	UFUNCTION(BlueprintCallable)
	void SaveHeavyAttack();

	void NewHeavyCombo();
	void AttackPaused();

	void Execution();

	UFUNCTION(BlueprintCallable)
	void StartWeaponCollision();
	UFUNCTION(BlueprintCallable)
	void EndWeaponCollision();

	
	void PerformLightAttack(int32 AttackIndex);
	void PerformHeavyAttack(int32 AttackIndex);
	void PerformAirCombo(int32 AttackIndex);
	void PerformHeavyCombo();
	void HeavyCombo(TArray<UAnimMontage*>& AttackMontages);

	bool SpecialAttack();
	bool CanLaunch();
	
	bool CanAttack() const;
	bool CanJumpFunc() const;
	UFUNCTION(BlueprintCallable,BlueprintPure)
	bool BothInAirOrNot();
	
	void ResetLightAttackVar();
	void ResetHeavyAttackVar();
	void ResetAirComboVar();

	void TargetLock();
	void SoftLockOn();

	
	void DoubleJump();
	void JustJumpedFalse();
	virtual void Landed(const FHitResult& Hit) override;
	void ResetDoubleJump();
	
	
	void Dodge();
	void PerformDodge();
	bool CanDodge() const;
	UFUNCTION(BlueprintCallable)
	void SaveDodge();
	bool PlayDodgeAttack();
	void RotationForDodge();

	UFUNCTION(BlueprintCallable)
	void ResetState();
	
	
	void Crasher();
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void CrasherDown();
	
	void Buffer(float Buffer);
	float BufferAmount;
	void StopBuffer();
	UFUNCTION()
	void BufferTimelineTickCallBack();
	UPROPERTY()
	UTimelineComponent* BufferTimeline;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	UCurveFloat* BufferCurve;


	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void Launch();
	
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void RotationToTarget();
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void StopRotation01();
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void StopRotation02();
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TSubclassOf<AEnemy> Enemy;

	FDoOnce DoOnce = FDoOnce(false);

public:

	FTimerHandle PausedAttack;
	FTimerHandle JustJumpedTimer;
	
	TArray<TEnumAsByte<EStates>> AttackState;
	TArray<TEnumAsByte<EStates>> DodgeState;
	TArray<TEnumAsByte<EStates>> ActionStates;
	TArray<TEnumAsByte<EStates>> FinisherStates;
	TArray<TEnumAsByte<EStates>> DodgeFinisherStates;

	bool bDodgeAttackEnabled;
	
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TEnumAsByte<EStates> CurrentState;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> LightAttackCombo;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	int32 LightAttackIndex;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bSaveLightAttack;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bSaveHeavyAttack;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> HeavyAttackMontages;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	int32 HeavyAttackIndex;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* DodgeMontage;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* DodgeAttack;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* LaunchUp;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* StingerMontage;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* CrasherMontage;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* DoubleJumpMontage;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	UAnimMontage* Execute;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bSaveDodge;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bHeavyAttackPaused;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	int32 NewHeavyComboIndex;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> HCombo01;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> HCombo02;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> HCombo03;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UClass*> CanLandedClasses;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bDoubleJump;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<AActor*> AlreadyHitActor;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bActiveCollision;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TSubclassOf<UDamageType> DamageType;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bTargeting;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	AActor* TargetActor;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	AActor* SoftTargetActor;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bJustJumped;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	FVector DesiredLaunchLocation;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bLaunched;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UAnimMontage*> AirComboMontage;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	int32 AirComboIndex;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	bool bLightInputHeld;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	FVector GroundLocation;

};

