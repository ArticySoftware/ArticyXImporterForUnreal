//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ArticyGlobalVariablesWorldSubsystem.generated.h"

class UArticyGlobalVariables;

/**
 * Keeps the runtime global variable clones outered to a world alive for as long as that world exists.
 *
 * A clone is otherwise reachable only through static weak pointers, and an outer does not keep its inners alive,
 * so without an owner the garbage collector discards the clone - and every variable value in it - on its next pass.
 */
UCLASS()
class UArticyGlobalVariablesWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Keeps the clone alive until it is released or the world goes away. */
	void KeepAlive(UArticyGlobalVariables* RuntimeClone) { Clones.AddUnique(RuntimeClone); }

	/** Stops keeping the clone alive. */
	void Release(UArticyGlobalVariables* RuntimeClone) { Clones.RemoveSingleSwap(RuntimeClone); }

private:
	UPROPERTY()
	TArray<TObjectPtr<UArticyGlobalVariables>> Clones;
};
