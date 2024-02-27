
#include "SlashCharacter.h"

#include "Enemy.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "E_States.h"
#include "InputActionValue.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ASlashCharacter::ASlashCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); 


	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;


	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; 
	CameraBoom->bUsePawnControlRotation = true; 


	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 

	Weapon = CreateDefaultSubobject<UStaticMeshComponent>("WeaponMesh");
	Weapon->SetupAttachment(GetMesh(),FName("WeaponSocket"));

	AttackState.Add(EStates::Attack);
	DodgeState.Add(EStates::Dodge);

	ActionStates.Add(EStates::Attack);
	ActionStates.Add(EStates::Dodge);

	FinisherStates.Add(EStates::Attack);
	FinisherStates.Add(EStates::Dodge);
	FinisherStates.Add(EStates::Finisher);

	DodgeFinisherStates.Add(EStates::Dodge);
	DodgeFinisherStates.Add(EStates::Finisher);

}

void ASlashCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}


	FOnTimelineFloatStatic OnBufferTimelineTickCallBack;
	BufferTimeline = NewObject<UTimelineComponent>(this, FName("BufferTimeline"));
	BufferTimeline->SetTimelineLength(0.25f);
	OnBufferTimelineTickCallBack.BindUFunction(this,TEXT("BufferTimelineTickCallBack"));
	BufferTimeline->AddInterpFloat(BufferCurve,OnBufferTimelineTickCallBack);
}

void ASlashCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActiveCollision)
	{
		TArray<FHitResult> OutHit;

		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);
	
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		//ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery3);
		
		bool bCollision = UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(),
			Weapon->GetSocketLocation(FName("Start")),
			Weapon->GetSocketLocation(FName("End")),20.0f,ObjectTypes,false,IgnoreActors,EDrawDebugTrace::None,OutHit,true);

		if (bCollision)
		{
			for (auto Hit : OutHit)
			{
				if (Hit.GetActor())
				{
					if (!AlreadyHitActor.Contains(Hit.GetActor()) && Hit.GetActor()->GetClass() == Enemy)
					{
						AlreadyHitActor.AddUnique(Hit.GetActor());
						//applyDamage
						Hit.GetActor()->TakeDamage(1.0f,FDamageEvent(DamageType),GetController(),this);
						
					}
				}
			}
		}
	}


	if (bTargeting && IsValid(TargetActor))
	{
		if (GetDistanceTo(TargetActor) <= 1000.0f)
		{
			AEnemy* TargetEnemy = Cast<AEnemy>(TargetActor);
			bool bPickA = TargetEnemy->GetCharacterMovement()->IsFalling() || TargetEnemy->GetCharacterMovement()->IsFlying();
			
			FRotator NewRotation = UKismetMathLibrary::RInterpTo(GetController()->K2_GetActorRotation(),
				UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),
				                                       TargetActor->GetActorLocation() -
				                                       UKismetMathLibrary::SelectVector(
					                                       FVector(0, 0, 80), FVector(0, 0, 30),bPickA)),
				UGameplayStatics::GetWorldDeltaSeconds(GetWorld()),5.0f);
			
			GetController()->SetControlRotation(NewRotation);
		}
		else
		{
			bTargeting = false;
			TargetActor = nullptr;
		}
	}

}

void ASlashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAction("LeftMouse",IE_Pressed,this,&ASlashCharacter::LightAttackPressed);
	PlayerInputComponent->BindAction("LeftMouse",IE_Released,this,&ASlashCharacter::LightAttackReleased);
	PlayerInputComponent->BindAction("RightMouse",IE_Pressed,this,&ASlashCharacter::HeavyAttack);
	PlayerInputComponent->BindAction("TargetLock",IE_Pressed,this,&ASlashCharacter::TargetLock);
	PlayerInputComponent->BindAction("Dodge",IE_Pressed,this,&ASlashCharacter::Dodge);
	PlayerInputComponent->BindAction("Execute",IE_Pressed,this,&ASlashCharacter::Execution);
	
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASlashCharacter::DoubleJump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

}

void ASlashCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASlashCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void ASlashCharacter::SetState(const TEnumAsByte<EStates> NewState)
{
	if (NewState != CurrentState)
	{
		CurrentState = NewState;
	}
}

TEnumAsByte<EStates> ASlashCharacter::GetState() const
{
	return  CurrentState;
}

bool ASlashCharacter::IsStateEqualToAny(const TArray<TEnumAsByte<EStates>>& StateToCheck) const
{
	return  StateToCheck.Contains(CurrentState);
}



void ASlashCharacter::LightAttackPressed()
{
	bLightInputHeld = true;
	bSaveDodge = false;
	bSaveHeavyAttack = false;
	if (IsStateEqualToAny(ActionStates))
	{
		bSaveLightAttack = true;
	}
	LightAttackEvent();
}

void ASlashCharacter::LightAttackReleased()
{
	bLightInputHeld = false;
}

void ASlashCharacter::LightAttackEvent()
{
	if (!PlayDodgeAttack())
	{
		if (CanAttack())
		{
			if (bTargeting)
			{
				if (SpecialAttack())  //CanLaunch
				{
				
				}
				else
				{
					ResetHeavyAttackVar();
					RotationToTarget();
					PerformLightAttack(LightAttackIndex);
				}
			}
			else
			{
				ResetHeavyAttackVar();
				RotationToTarget();
				PerformLightAttack(LightAttackIndex);
			}
		}
		else
		{
			if (bLaunched)
			{
				if (!IsStateEqualToAny(FinisherStates)) 
				{
					GetCharacterMovement()->SetMovementMode(MOVE_Flying);
					RotationToTarget();
					PerformAirCombo(AirComboIndex);
				}
			}
		}
	}
	
	/*if (CanAttack())
	{
		if (bTargeting)
		{
			if (SpecialAttack())  //CanLaunch
			{
				
			}
			else
			{
				ResetHeavyAttackVar();
				RotationToTarget();
				PerformLightAttack(LightAttackIndex);
			}
		}
		else
		{
			ResetHeavyAttackVar();
			RotationToTarget();
			PerformLightAttack(LightAttackIndex);
		}
	}
	else
	{
		if (bLaunched)
		{
			if (!IsStateEqualToAny(ActionStates))
			{
				GetCharacterMovement()->SetMovementMode(MOVE_Flying);
				RotationToTarget();
				PerformAirCombo(AirComboIndex);
			}
		}
	}*/
}

void ASlashCharacter::SaveLightAttack()
{
	if (bSaveLightAttack)
	{
		bSaveLightAttack = false;

		if (IsStateEqualToAny(DodgeState))
		{
			bDodgeAttackEnabled = true;
			
			if (IsStateEqualToAny(AttackState))
			{
				SetState(Nothing);
				LightAttackEvent();
			}
			else
			{
				LightAttackEvent();
			}
		}
		else
		{
			if (IsStateEqualToAny(AttackState))
			{
				SetState(Nothing);
				LightAttackEvent();
			}
			else
			{
				LightAttackEvent();
			}
		}

		/*if (IsStateEqualToAny(AttackState))
		{
			SetState(Nothing);
			LightAttackEvent();
		}
		else
		{
			LightAttackEvent();
		}*/
	}
}



void ASlashCharacter::HeavyAttack()
{
	bSaveDodge = false;
	bSaveLightAttack= false;

	if (IsStateEqualToAny(ActionStates))
	{
		bSaveHeavyAttack = true;
	}
	else
	{
		HeavyAttackEvent();
	}
}

void ASlashCharacter::HeavyAttackEvent()
{
	if (CanAttack())
	{
		GetWorld()->GetTimerManager().ClearTimer(PausedAttack);
		bHeavyAttackPaused = false;
		ResetLightAttackVar();
		RotationToTarget();
		PerformHeavyAttack(HeavyAttackIndex);
	}
}

void ASlashCharacter::SaveHeavyAttack()
{
	if (bSaveHeavyAttack)
	{
		bSaveHeavyAttack = false;
		if (!bLaunched)
		{
			if (IsStateEqualToAny(AttackState))
			{
				SetState(Nothing);
				if (!bHeavyAttackPaused)
				{
					HeavyAttackEvent();
				}
				else
				{
					NewHeavyCombo();
				}
			}
			else
			{
				if (!bHeavyAttackPaused)
				{
					HeavyAttackEvent();
				}
				else
				{
					NewHeavyCombo();
				}
			}
		}
		else
		{
			RotationToTarget();
			Crasher();
		}
	}
}

void ASlashCharacter::NewHeavyCombo()
{
	if (!IsStateEqualToAny(AttackState))
	{
		PerformHeavyCombo();
	}
}

void ASlashCharacter::AttackPaused()
{
	bHeavyAttackPaused = true;
}

void ASlashCharacter::Execution()
{
	if (!IsStateEqualToAny(DodgeFinisherStates)) 
	{
		if (bTargeting && IsValid(TargetActor))
		{
			if (GetDistanceTo(TargetActor) <= 100.0f) // Do Other checks here like health  (better solution motion warping)
			{
				StopRotation01();
				StopRotation02();
				StopBuffer();
				AEnemy* TargetEnemy = Cast<AEnemy>(TargetActor);

				TargetEnemy->EnemyFinisher(this);
				SetState(Finisher);
				SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),TargetActor->GetActorLocation()));
				PlayAnimMontage(Execute);
				bTargeting = false;
				TargetActor = nullptr;

				GEngine->AddOnScreenDebugMessage(-1,7.f,FColor::Purple,TEXT("Execution"));
			}
		}
	}
}


void ASlashCharacter::StartWeaponCollision()
{
	AlreadyHitActor.Empty();
	bActiveCollision = true;
}

void ASlashCharacter::EndWeaponCollision()
{
	bActiveCollision = false;
}



bool ASlashCharacter::CanAttack() const
{
	return !GetCharacterMovement()->IsFalling() && !GetCharacterMovement()->IsFlying() && !IsStateEqualToAny(FinisherStates); 
}



bool ASlashCharacter::CanJumpFunc() const
{
	return !IsStateEqualToAny(FinisherStates); 
}

bool ASlashCharacter::BothInAirOrNot()
{
	const AEnemy* TargetEnemy = Cast<AEnemy>(TargetActor);
	return ((GetCharacterMovement()->IsFlying() || GetCharacterMovement()->IsFalling()) && (TargetEnemy->
			GetCharacterMovement()->IsFlying() || TargetEnemy->GetCharacterMovement()->IsFalling()))
	|| ((!GetCharacterMovement()->IsFlying() && !GetCharacterMovement()->IsFalling()) && (!TargetEnemy->GetCharacterMovement()
		->IsFlying() && !TargetEnemy->GetCharacterMovement()->IsFalling()));
}


void ASlashCharacter::PerformLightAttack(int32 AttackIndex)
{
	UAnimMontage* L_AttackMontage = LightAttackCombo[AttackIndex];

	if (IsValid(L_AttackMontage))
	{
		StopBuffer();
		Buffer(3.0f);
		SetState(Attack);
		SoftLockOn();
		PlayAnimMontage(L_AttackMontage);
		LightAttackIndex = ++LightAttackIndex;

		if (LightAttackIndex >= LightAttackCombo.Num())
		{
			LightAttackIndex = 0;
		}
	}
}

void ASlashCharacter::PerformHeavyAttack(int32 AttackIndex)
{
	UAnimMontage* H_AttackMontage = HeavyAttackMontages[AttackIndex];

	if (IsValid(H_AttackMontage))
	{
		StopBuffer();
		Buffer(3.0f);
		SetState(Attack);
		SoftLockOn();
		PlayAnimMontage(H_AttackMontage);
		
		GetWorld()->GetTimerManager().SetTimer(PausedAttack,this,&ASlashCharacter::AttackPaused,0.5f);
		
		HeavyAttackIndex = ++HeavyAttackIndex;

		if (HeavyAttackIndex >= HeavyAttackMontages.Num())
		{
			HeavyAttackIndex = 0;
			
			GetWorld()->GetTimerManager().ClearTimer(PausedAttack);
			bHeavyAttackPaused = false;
		}
	}
}

void ASlashCharacter::PerformAirCombo(int32 AttackIndex)
{
	UAnimMontage* L_AttackMontage = AirComboMontage[AttackIndex];

	if (IsValid(L_AttackMontage))
	{
		StopBuffer();
		SetState(Attack);
		PlayAnimMontage(L_AttackMontage);
		AirComboIndex = ++AirComboIndex;

		if (AirComboIndex >= AirComboMontage.Num())
		{
			AirComboIndex = 0;
			bLaunched = false;
		}
	}
}

void ASlashCharacter::PerformHeavyCombo()
{
	GetWorld()->GetTimerManager().ClearTimer(PausedAttack);
	
	switch (HeavyAttackIndex-1)
	{
	case 0:
		HeavyCombo(HCombo01);
		break;
	case 1:
		HeavyCombo(HCombo02);
		break;
	case 2:
		HeavyCombo(HCombo03);
		break;
	default:
		break;
	}
}

void ASlashCharacter::HeavyCombo(TArray<UAnimMontage*>& AttackMontages)
{
	UAnimMontage* L_AttackMontage = AttackMontages[NewHeavyComboIndex];

	if (IsValid(L_AttackMontage))
	{
		RotationToTarget();
		StopBuffer();
		Buffer(3.0f);
		SetState(Attack);
		SoftLockOn();
		PlayAnimMontage(L_AttackMontage);
		NewHeavyComboIndex = ++NewHeavyComboIndex;

		if (NewHeavyComboIndex >= AttackMontages.Num())
		{
			NewHeavyComboIndex = 0;
			bHeavyAttackPaused = false;
		}
	}
}


bool ASlashCharacter::SpecialAttack()
{
	if (bTargeting && IsValid(TargetActor))
	{
		if (GetDistanceTo(TargetActor) > 300.f)
		{
			SetState(Attack);
			ResetLightAttackVar();
			ResetHeavyAttackVar();
			RotationToTarget();
			Buffer(25.0f);
			PlayAnimMontage(StingerMontage);
			return true;
		}

		if(GetDistanceTo(TargetActor) >15.f && GetDistanceTo(TargetActor) < 300.f )
		{
			RotationToTarget();
			SetState(Attack);
			PlayAnimMontage(LaunchUp);
			return true;	
		}
		
	}
	return false;
}

bool ASlashCharacter::CanLaunch()
{
	if (bTargeting)
	{
		RotationToTarget();
		SetState(Attack);
		PlayAnimMontage(LaunchUp);
		return true;
	}
	return false;
}


void ASlashCharacter::ResetLightAttackVar()
{
	LightAttackIndex = 0;
	bSaveLightAttack = false;
	bDodgeAttackEnabled = false;
}

void ASlashCharacter::ResetHeavyAttackVar()
{
	HeavyAttackIndex = 0;
	bSaveHeavyAttack = false;
	
	GetWorld()->GetTimerManager().ClearTimer(PausedAttack);
	bHeavyAttackPaused = false;
	NewHeavyComboIndex = 0;
}

void ASlashCharacter::ResetAirComboVar()
{
	AirComboIndex = 0;
	bLaunched = false;
}

void ASlashCharacter::ResetState()
{
	if (GetCharacterMovement()->IsFlying())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		SetState(Nothing);
		ResetLightAttackVar();
		ResetHeavyAttackVar();
		bSaveDodge = false;
		StopBuffer();
		StopRotation01();
		StopRotation02();
		ResetAirComboVar();
		SoftTargetActor = nullptr;
	}
	else
	{
		SetState(Nothing);
		ResetLightAttackVar();
		ResetHeavyAttackVar();
		bSaveDodge = false;
		StopBuffer();
		StopRotation01();
		StopRotation02();
		ResetAirComboVar();
		SoftTargetActor = nullptr;
	}
}



void ASlashCharacter::TargetLock()
{
	if (!bTargeting && !IsValid(TargetActor))
	{
		FHitResult OutHit;

		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);
	
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

		bool bHitTarget =UKismetSystemLibrary::SphereTraceSingleForObjects(GetWorld(), GetActorLocation(),
		                                                  GetActorLocation() + FollowCamera->GetForwardVector() *
		                                                  400.0f, 100.0f, ObjectTypes, false, IgnoreActors,
		                                                  EDrawDebugTrace::ForDuration, OutHit, true);

		if (bHitTarget)
		{
			if (IsValid(OutHit.GetActor()))
			{
				const AEnemy* EnemyTargeted= Cast<AEnemy>(OutHit.GetActor());
				if(!EnemyTargeted->bDead)
				{
					bTargeting = true;
					TargetActor = OutHit.GetActor();
				}
			}
		}
	}
	else
	{
		bTargeting = false;
		TargetActor = nullptr;
	}
}

void ASlashCharacter::SoftLockOn()
{
	if (!bTargeting && !UKismetSystemLibrary::IsValid(TargetActor))
	{
		FHitResult OutHit;

		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);
	
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		
		bool bTraceValue = UKismetSystemLibrary::SphereTraceSingleForObjects(GetWorld(), GetActorLocation(),
		                                                  GetActorLocation() + GetCharacterMovement()->
		                                                  GetLastInputVector() * 1000.0f,100.0f, ObjectTypes, false,
		                                                  IgnoreActors, EDrawDebugTrace::ForDuration,OutHit,true);

		if (bTraceValue)
		{
			if(IsValid(OutHit.GetActor()))
			{
				SoftTargetActor = OutHit.GetActor();
			}
		}
		else
		{
			SoftTargetActor = nullptr;
		}
	}
	else
	{
		SoftTargetActor = nullptr;
	}
}



void ASlashCharacter::DoubleJump()
{
	const FVector LaunchVelocity(0,0,800);

	if (CanJumpFunc())
	{
		if (!GetCharacterMovement()->IsFalling())
		{
			bJustJumped = true;
			Jump();

			GetWorld()->GetTimerManager().SetTimer(JustJumpedTimer, this,&ASlashCharacter::JustJumpedFalse, 0.2f, false);
		}
		else
		{
			if (DoOnce.Completed())
			{
				bJustJumped = false;
				bDoubleJump = true;
				PlayAnimMontage(DoubleJumpMontage);
				LaunchCharacter(LaunchVelocity,false,true);
			}
		}
	}
}

void ASlashCharacter::JustJumpedFalse()
{
	bJustJumped = false;
}

void ASlashCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (CanLandedClasses.Contains(Hit.GetActor()->GetClass()))
	{
		ResetDoubleJump();
		bDoubleJump =false;
		bLaunched = false;
	}
	GetWorld()->GetTimerManager().ClearTimer(JustJumpedTimer);
}

void ASlashCharacter::ResetDoubleJump()
{
	DoOnce.Reset();
}



void ASlashCharacter::Dodge()
{
	if (CanDodge())
	{
		PerformDodge();
	}
	else
	{
		bSaveDodge = true;
	}
}

void ASlashCharacter::PerformDodge()
{
	StopRotation01();
	StopRotation02();
	SoftTargetActor = nullptr;
	RotationForDodge();
	StopBuffer();
	Buffer(15);
	SetState(EStates::Dodge);
	PlayAnimMontage(DodgeMontage);
}

bool ASlashCharacter::CanDodge() const
{
	return !GetCharacterMovement()->IsFalling() && !IsStateEqualToAny(DodgeFinisherStates); 
}

void ASlashCharacter::SaveDodge()
{
	if (bSaveDodge)
	{
		bSaveDodge = false;

		if (IsStateEqualToAny(DodgeState))
		{
			SetState(Nothing);
			PerformDodge();
		}
		else
		{
			PerformDodge();
		}
	}
}

bool ASlashCharacter::PlayDodgeAttack()
{
	if (bDodgeAttackEnabled)
	{
		bDodgeAttackEnabled = false;
		SetState(Attack);
		RotationToTarget();
		PlayAnimMontage(DodgeAttack);
		return true;
	}
	return false;
}

void ASlashCharacter::RotationForDodge()
{
	if (GetCharacterMovement()->GetLastInputVector() != FVector(0))
	{
		SetActorRotation(UKismetMathLibrary::MakeRotFromX(GetCharacterMovement()->GetLastInputVector()));
	}
}


void ASlashCharacter::Crasher()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	SetState(Attack);
	PlayAnimMontage(CrasherMontage);

	FHitResult OutHit;

	const TArray<AActor*> IgnoreActors;
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	/*ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2);*/
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	
	bool bTraceValue = UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), GetActorLocation(),
	                                                UKismetMathLibrary::MakeVector(
		                                                GetActorLocation().X, GetActorLocation().Y,
		                                                GetActorLocation().Z - 1000000000000.0f), ObjectTypes,false,IgnoreActors ,
		                                                EDrawDebugTrace::None,OutHit,true);

	if (bTraceValue)
	{
		GroundLocation = OutHit.ImpactPoint;
	}
}


void ASlashCharacter::Buffer(const float Buffer)
{
	BufferTimeline->PlayFromStart();
	BufferAmount = Buffer;
}

void ASlashCharacter::StopBuffer()
{
	BufferTimeline->Stop();
}

void ASlashCharacter::BufferTimelineTickCallBack()
{
	const float FloatCurveVal = BufferCurve->GetFloatValue(BufferTimeline->GetPlaybackPosition());
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),GetActorForwardVector()*BufferAmount+GetActorLocation(),FloatCurveVal),true);
}




/*
void ASlashCharacter::CrasherDown()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	DownTimeline->PlayFromStart();
}

void ASlashCharacter::DownTimelineTickCallBack()
{
	const float Speed = DownCurve->GetFloatValue(DownTimeline->GetPlaybackPosition());
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),GroundLocation,Speed),true);
}



void ASlashCharacter::Launch()
{
	if (bLightInputHeld)
	{
		bLaunched =true;
		DesiredLaunchLocation = GetActorLocation();
		CharacterLaunch->PlayFromStart();
	}
}

void ASlashCharacter::LaunchTimelineTickCallBack()
{
	const float LaunchSpeed = BufferCurve->GetFloatValue(BufferTimeline->GetPlaybackPosition());
	
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),
	                                           UKismetMathLibrary::MakeVector(
		                                           DesiredLaunchLocation.X, DesiredLaunchLocation.Y,
		                                           DesiredLaunchLocation.Z + 400.f), LaunchSpeed), true);
}

void ASlashCharacter::RotationToTarget()
{
	if (UKismetSystemLibrary::IsValid(TargetActor))
	{
		RotationTimeline->PlayFromStart();
	}

	else
	{
		if (UKismetSystemLibrary::IsValid(SoftTargetActor))
		{
			SoftLockRotation->PlayFromStart();
		}
	}
}

void ASlashCharacter::RotationTimelineTickCallBack()
{
	const float Speed = RotationCurve->GetFloatValue(RotationTimeline->GetPlaybackPosition());

	if (UKismetSystemLibrary::IsValid(TargetActor))
	{
		FRotator ActorRotator = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),TargetActor->GetActorLocation());
		
		SetActorRotation(UKismetMathLibrary::RLerp(GetActorRotation(),
								  UKismetMathLibrary::MakeRotator(ActorRotator.Roll, GetActorRotation().Pitch,
																  ActorRotator.Yaw), Speed, false));
		
	}
	else
	{
		StopRotation01();
	}
}

void ASlashCharacter::SoftLockRotationTimelineTickCallBack()
{
	const float Speed = SoftLockRotationCurve->GetFloatValue(SoftLockRotation->GetPlaybackPosition());

	if (UKismetSystemLibrary::IsValid(SoftTargetActor))
	{
		FRotator ActorRotator = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),SoftTargetActor->GetActorLocation());
		
		SetActorRotation(UKismetMathLibrary::RLerp(GetActorRotation(),
								  UKismetMathLibrary::MakeRotator(ActorRotator.Roll, GetActorRotation().Pitch,
																  ActorRotator.Yaw), Speed, false));
		
	}
	else
	{
		StopRotation02();
	}
	
}

void ASlashCharacter::StopRotation01()
{
	RotationTimeline->Stop();
}

void ASlashCharacter::StopRotation02()
{
	SoftLockRotation->Stop();
}*/