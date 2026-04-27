//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Interfaces/ArticyInputPinsProvider.h"

#include "ArticyFlowPlayer.h"

/**
 * @brief Tries to submerge into InputPins and explore connections.
 *
 * This method attempts to explore input pins of a flow node by checking for any
 * connected pins and exploring them using the provided player. It is used primarily
 * when starting an exploration at a flow node.
 *
 * @param Player A pointer to the ArticyFlowPlayer instance managing the exploration.
 * @param OutBranches An array to store the resulting branches from the exploration.
 * @param Depth The current depth of exploration.
 * @param bForceShadowed A boolean flag indicating whether to force shadowed exploration.
 *
 * @return Returns true if successful in submerging into InputPins; otherwise, false.
 */
bool IArticyInputPinsProvider::TrySubmerge(class UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth, const bool bForceShadowed)
{
	bool bSubmerged = false;

	auto inPins = GetInputPinsPtr();
	if (ensure(inPins) && inPins->Num() > 0)
	{
		// If there is more than one pin or the single pin has more connections,
		// it must be a shadowed explore
		const auto bShadowed = bForceShadowed
			|| inPins->Num() > 1
			|| ((*inPins)[0] && (*inPins)[0]->Connections.Num() > 1);

		// Submerge!
		for (auto pin : *inPins)
		{
			// Skip pins with no connections, since non-submergeable pins should not exist if
			// at least one of the other pins can be submerged.
			// If none of the pins has connections, TrySubmerge will fail anyways, and the owner will be explored instead.
			if (ensure(pin) && pin->Connections.Num() > 0)
			{
				bSubmerged = true;
				OutBranches.Append(Player->Explore(pin, bShadowed, Depth + 1));
			}
		}
	}

	return bSubmerged;
}

/**
 * @brief Retrieves a pointer to the InputPins array.
 *
 * This function returns a pointer to the array of input pins, if available, for this object.
 *
 * @return A pointer to an array of UArticyInputPin objects, or nullptr if not found.
 */
const TArray<UArticyInputPin*>* IArticyInputPinsProvider::GetInputPinsPtr() const
{
	const static auto name = FName("InputPins");
	return Cast<IArticyReflectable>(this)->GetPropPtr<TArray<UArticyInputPin*>>(name);
}

/**
 * @brief Implementation of the GetInputPins function.
 *
 * This function provides the actual implementation for retrieving the array of input pins.
 *
 * @return An array of UArticyInputPin objects.
 */
TArray<UArticyInputPin*> IArticyInputPinsProvider::GetInputPins_Implementation() const
{
	auto Pins = GetInputPinsPtr();
	if (Pins)
		return *Pins;
	else
		return TArray<UArticyInputPin*>();
}
