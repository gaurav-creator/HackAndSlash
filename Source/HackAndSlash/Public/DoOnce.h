#pragma once

#include "DoOnce.generated.h"

USTRUCT(BlueprintType)
struct FDoOnce
{
	GENERATED_BODY();

	bool bDoOnce;

	FORCEINLINE FDoOnce();
	explicit FORCEINLINE FDoOnce(bool bStartedClosed);

	FORCEINLINE void Reset() {bDoOnce = true;}

	FORCEINLINE bool Completed()
	{
		if (bDoOnce)
		{
			bDoOnce = false;
			return true;
		}
		return false;
	}
};

FORCEINLINE FDoOnce::FDoOnce() : bDoOnce(false)
{
}

FORCEINLINE FDoOnce::FDoOnce(const bool bStartedClosed) : bDoOnce(!bStartedClosed)
{
}
