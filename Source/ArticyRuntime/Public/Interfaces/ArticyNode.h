//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObject.h"
#include "ArticyFlowObject.h"
#include "ArticyInputPinsProvider.h"
#include "ArticyPausableType.h"
#include "ArticyOutputPinsProvider.h"

#include "ArticyNode.generated.h"

/**
 * @class UArticyNode
 * @brief Base class for all nodes (fragments) in the flow.
 *
 * This class serves as the base for all nodes within the flow system,
 * implementing both input and output pin interfaces to allow for node exploration.
 */
UCLASS(abstract)
class ARTICYRUNTIME_API UArticyNode : public UArticyObject, public IArticyInputPinsProvider, public IArticyOutputPinsProvider
{
	GENERATED_BODY()

public:

	/**
	 * @brief Gets the type of this node.
	 *
	 * This function returns the type of the node, which is pausable. It is intended
	 * to be overridden in derived classes.
	 *
	 * @return The type of the node as EArticyPausableType.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Flow")
	EArticyPausableType GetType() override { ensure(false); return static_cast<EArticyPausableType>(0); }

	/**
	 * @brief Explores the node's output pins.
	 *
	 * This function explores the node by continuing on its output pins, allowing the
	 * ArticyFlowPlayer to explore the connected branches.
	 *
	 * @param Player A pointer to the ArticyFlowPlayer managing the exploration.
	 * @param OutBranches An array to store the resulting branches from the exploration.
	 * @param Depth The current depth of exploration.
	 */
	void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override;
};
