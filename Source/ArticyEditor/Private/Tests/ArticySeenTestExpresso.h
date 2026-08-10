//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "ArticyExpressoScripts.h"
#include "ArticyGlobalVariables.h"
#include "ArticySeenTestExpresso.generated.h"

/**
 * @brief Hand-authored stand-in for a generated ExpressoScripts class.
 *
 * Registers the exact condition the importer emits for a bare "seen" keyword, so tests can
 * drive a real Evaluate() through the real getSeenCounter() path without needing imported
 * content that happens to contain a seen condition. SetGV/GetGV mirror the generated class,
 * which is what makes getSeenCounter() able to resolve global variables while evaluating.
 */
UCLASS()
class UArticySeenTestExpresso : public UArticyExpressoScripts
{
	GENERATED_BODY()

public:

	/** The fragment text this fixture answers to; its hash keys the condition. */
	static int32 SeenFragmentHash() { return (int32)GetTypeHash(FString(TEXT("seen"))); }

	UArticySeenTestExpresso()
	{
		// exactly what AddScriptFragment generates for the bare "seen" keyword
		Conditions.Add(SeenFragmentHash(), [&] { return ConditionOrTrue(getSeenCounter() > 0); });
	}

	void SetGV(UArticyGlobalVariables* GV) const override { ActiveGV = GV; }
	UArticyGlobalVariables* GetGV() override { return ActiveGV.Get(); }

private:

	mutable TWeakObjectPtr<UArticyGlobalVariables> ActiveGV = nullptr;
};
