//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/ArticyObjectWithPosition.h"
#include "ArticyTestPositional.generated.h"

// Minimal test implementing IArticyObjectWithPosition. Overrides GetPosition() 
// to return a directly-settable value, bypassing the reflected "Position" 
// property lookup, so branch-sorter logic can be tested in isolation.
UCLASS()
class UArticyTestPositional : public UObject, public IArticyObjectWithPosition
{
	GENERATED_BODY()

public:
	FVector2D TestPosition = FVector2D::ZeroVector;

	virtual FVector2D& GetPosition() override { return TestPosition; }
};
