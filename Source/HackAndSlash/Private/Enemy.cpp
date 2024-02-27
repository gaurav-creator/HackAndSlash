
#include "Enemy.h"

#include <d3d10.h>

#include "DTMain.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


AEnemy::AEnemy(): HR_Left(nullptr), HR_Right(nullptr), HR_Middle(nullptr), HR_KnockBack(nullptr), AirHit(nullptr),
                  KnockDownCrash(nullptr), BufferAmount(0),
                  BufferTimeline(nullptr),
                  BufferEnemyCurve(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemy::PlayHitReact(TEnumAsByte<EDamageTypes> DamageType, AActor* DamageCauser)
{
	if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying())
	{
		bAirAttacked = true;
		switch (DamageType) {
		case None:
			break;
		case Left:
			SetActorLocation(UKismetMathLibrary::MakeVector(GetActorLocation().X,GetActorLocation().Y,DamageCauser->GetActorLocation().Z),true);
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			PlayAnimMontage(AirHit);
			break;
		case Right:
			SetActorLocation(UKismetMathLibrary::MakeVector(GetActorLocation().X,GetActorLocation().Y,DamageCauser->GetActorLocation().Z),true);
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			PlayAnimMontage(AirHit);
			break;
		case Middle:
			SetActorLocation(UKismetMathLibrary::MakeVector(GetActorLocation().X,GetActorLocation().Y,DamageCauser->GetActorLocation().Z),true);
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			PlayAnimMontage(AirHit);
			break;
		case KnockBack:
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			Buffer(-20.0f);
			PlayAnimMontage(HR_AirKnockBack);
			bAirAttacked = false;
			bAirKnockBack =true;
			break;
		case EDamageTypes::Launch:
			break;
		case EDamageTypes::KnockDown:
			bAirAttacked =false;
			KnockDownCrasher();
			break;
		default:
			break;
		}
	}

	else
	{
		switch (DamageType) {
		case None:
			break;
		case Left:
			Buffer(-6.f);
			PlayAnimMontage(HR_Left);
			break;
		case Right:
			Buffer(-6.f);
			PlayAnimMontage(HR_Right);
			break;
		case Middle:
			Buffer(-6.f);
			PlayAnimMontage(HR_Middle);
			break;
		case KnockBack:
			Buffer(-10.f);
			PlayAnimMontage(HR_KnockBack);
			break;
		case EDamageTypes::Launch:
			PlayAnimMontage(AirHit);
			bAirAttacked = true;
			Launch();
			break;
		case EDamageTypes::KnockDown:
			PlayAnimMontage(KnockDownCrash);
			break;
		default:
			break;
		}
	}
}

void AEnemy::ResetEnemy() 
{
	if (bAirAttacked)
	{
		PlayAnimMontage(Falling);
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
	else
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

void AEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (CanLandedClasses.Contains(Hit.GetActor()->GetClass()))
	{
		if (bAirAttacked)
		{
			PlayAnimMontage(K_Landed);
			bAirAttacked = false;
		}
		else
		{
			if (bAirKnockBack)
			{
				PlayAnimMontage(AirKnockBackLanded);
				bAirKnockBack = false;
			}
		}
	}
}

void AEnemy::EnemyFinisher(AActor* Player)
{
	bDead = true;
	const ACharacter* Character= UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),Character->GetActorLocation()));
	PlayAnimMontage(EnemyExecution);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	FOnTimelineFloatStatic OnBufferTimelineTickCallBack;
	BufferTimeline = NewObject<UTimelineComponent>(this, FName("BufferTimeline"));
	BufferTimeline->SetTimelineLength(0.25f);
	OnBufferTimelineTickCallBack.BindUFunction(this,TEXT("BufferTimelineTickCallBack"));
	BufferTimeline->AddInterpFloat(BufferEnemyCurve,OnBufferTimelineTickCallBack);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AEnemy::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	// If you need the DamageType object like in blueprint, this is how you do it:
	UDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();

	if (!bDead)
	{
		const UDTMain* MainDamage = Cast<UDTMain>(DamageTypeCDO);
        
		const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),DamageCauser->GetActorLocation());
        	
		SetActorRotation(UKismetMathLibrary::MakeRotator(LookAtRotation.Roll,GetActorRotation().Pitch,LookAtRotation.Yaw));
        	
		PlayHitReact(MainDamage->DamageType,DamageCauser);
	}
	
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemy::Buffer(float Buffer)
{
	BufferTimeline->PlayFromStart();
	BufferAmount = Buffer;
}

void AEnemy::StopBuffer()
{
	BufferTimeline->Stop();
}

void AEnemy::BufferTimelineTickCallBack()
{
	const float FloatCurveVal = BufferEnemyCurve->GetFloatValue(BufferTimeline->GetPlaybackPosition());
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),GetActorForwardVector()*BufferAmount+GetActorLocation(),FloatCurveVal),true);
}

/*
void AEnemy::Launch()
{
	DesiredLaunchLocation = GetActorLocation();
	EnemyLaunch->PlayFromStart();
}

void AEnemy::StopLaunch()
{
	EnemyLaunch->Stop();
}

void AEnemy::LaunchTimelineTickCallBack()
{
	const float LaunchSpeed = EnemyLaunchCurve->GetFloatValue(EnemyLaunch->GetPlaybackPosition());
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),
											   UKismetMathLibrary::MakeVector(
												   DesiredLaunchLocation.X, DesiredLaunchLocation.Y,
												   DesiredLaunchLocation.Z + 400.f), LaunchSpeed), true);
}

void AEnemy::KnockDownCrasher()
{
	FHitResult OutHit;

	const TArray<AActor*> IgnoreActors;
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2);
	
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), GetActorLocation(),
	                                                UKismetMathLibrary::MakeVector(
		                                                GetActorLocation().X, GetActorLocation().Y,
		                                                GetActorLocation().Z - 10000000000.0f), ObjectTypes, false,
	                                                IgnoreActors,EDrawDebugTrace::ForDuration, OutHit, true);

	EnemyGroundLocation = OutHit.ImpactPoint;
	StopLaunch();
	PlayAnimMontage(KnockDownCrash);
	KnockDown->PlayFromStart();
}

void AEnemy::KnockDownCrasherTimelineTickCallBack()
{
	const float Speed = KnockDownCrasherCurve->GetFloatValue(KnockDown->GetPlaybackPosition());
	SetActorLocation(UKismetMathLibrary::VLerp(GetActorLocation(),EnemyGroundLocation,Speed),true);
}
*/