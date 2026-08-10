//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyPrimitive.h"
#include "ArticyBaseTypes.h"
#include "Interfaces/ArticyFlowObject.h"
#include "ArticyTestFlowObject.generated.h"

/**
 * @brief Minimal flow object with a settable id.
 *
 * The seen-counter and fallback stores key on UArticyPrimitive::GetId(), which is only
 * ever written during import. Freshly constructed objects all share the null id, so tests
 * need a primitive whose id they can set to tell two nodes apart without imported content.
 */
UCLASS()
class UArticyTestFlowObject : public UArticyPrimitive, public IArticyFlowObject
{
	GENERATED_BODY()

public:

	/** Assigns the id this object is tracked under. */
	void SetTestId(const FArticyId& InId) { Id = InId; }

	//~ IArticyFlowObject
	virtual EArticyPausableType GetType() override { return EArticyPausableType::FlowFragment; }
	virtual void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override {}
};
