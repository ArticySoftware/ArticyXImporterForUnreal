//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "ArticyExpressoScripts.h"
#include "ArticyCountingTestExpresso.generated.h"

/**
 * @brief Expresso stand-in that counts how often one instruction fragment is executed.
 *
 * Registers a single instruction under the hash of CountedFragment(). A pin whose Text is set
 * to that fragment can then be run through the real UArticyOutputPin::Execute path while the
 * test observes how many times the instruction actually fires, independent of what the
 * imported project's own instructions do (the demo content only has idempotent assignments,
 * which cannot tell one execution from two).
 */
UCLASS()
class UArticyCountingTestExpresso : public UArticyExpressoScripts
{
	GENERATED_BODY()

public:

	/** The fragment text the counted instruction answers to; assign it to a pin's Text. */
	static FString CountedFragment() { return TEXT("ArticyTest.CountExecutions()"); }

	UArticyCountingTestExpresso()
	{
		Instructions.Add(GetTypeHash(CountedFragment()), [this] { ++ExecutionCount; });
	}

	/** How often the counted instruction has run since the last reset. */
	int32 ExecutionCount = 0;
};
