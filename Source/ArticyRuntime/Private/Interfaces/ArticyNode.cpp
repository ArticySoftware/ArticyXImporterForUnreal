//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Interfaces/ArticyNode.h"

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
void UArticyNode::Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth)
{
	// Default implementation: continue on output pins
	IArticyOutputPinsProvider::Explore(Player, OutBranches, Depth + 1);
}
