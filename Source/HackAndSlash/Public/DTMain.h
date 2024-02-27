// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EDamageTypes.h"
#include "GameFramework/DamageType.h"
#include "DTMain.generated.h"

/**
 * 
 */
UCLASS()
class HACKANDSLASH_API UDTMain : public UDamageType
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Default")
	TEnumAsByte<EDamageTypes> DamageType;
};
