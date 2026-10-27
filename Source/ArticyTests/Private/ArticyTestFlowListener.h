//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyBranch.h"
#include "Interfaces/ArticyFlowObject.h"
#include "ArticyTestFlowListener.generated.h"

/**
 * @brief Counts the flow player callbacks it is bound to.
 *
 * The flow player reports through dynamic multicast delegates, which can only be bound to a
 * UFUNCTION, so tests that check whether a callback fired need an object to bind.
 */
UCLASS()
class UArticyTestFlowListener : public UObject
{
	GENERATED_BODY()

public:

	/** How often OnPlayerPaused fired. */
	int32 PausedCount = 0;

	/** How often OnBranchesUpdated fired. */
	int32 BranchesUpdatedCount = 0;

	/** The node reported by the last OnPlayerPaused. */
	TScriptInterface<IArticyFlowObject> LastPausedOn;

	/** The number of branches reported by the last OnBranchesUpdated. */
	int32 LastBranchCount = 0;

	/** Records an OnPlayerPaused broadcast. */
	UFUNCTION()
	void HandlePlayerPaused(TScriptInterface<IArticyFlowObject> PausedOn)
	{
		++PausedCount;
		LastPausedOn = PausedOn;
	}

	/** Records an OnBranchesUpdated broadcast. */
	UFUNCTION()
	void HandleBranchesUpdated(const TArray<FArticyBranch>& AvailableBranches)
	{
		++BranchesUpdatedCount;
		LastBranchCount = AvailableBranches.Num();
	}
};
