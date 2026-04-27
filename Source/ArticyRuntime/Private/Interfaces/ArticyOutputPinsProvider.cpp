//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Interfaces/ArticyOutputPinsProvider.h"

#include "ArticyBranch.h"
#include "ArticyFlowPlayer.h"

/**
 * @brief Explores the output pins of the provider.
 *
 * This function explores the provider's output pins, allowing the ArticyFlowPlayer
 * to explore the connected branches. If there are no output pins, it adds a dead-end branch.
 *
 * @param Player A pointer to the ArticyFlowPlayer managing the exploration.
 * @param OutBranches An array to store the resulting branches from the exploration.
 * @param Depth The current depth of exploration.
 */
void IArticyOutputPinsProvider::Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth)
{
	auto pins = GetOutputPinsPtr();
	if (ensure(pins) && pins->Num() > 0)
	{
		// Shadow needed?
		const auto bShadowed = pins->Num() > 1;

		for (auto pin : *pins)
			OutBranches.Append(Player->Explore(pin, bShadowed, Depth + 1));
	}
	else
	{
		// DEAD-END!
		OutBranches.Add(FArticyBranch{});
	}
}

/**
 * @brief Retrieves the output pins of the provider.
 *
 * This function returns a pointer to the array of output pins associated with the provider.
 *
 * @return A pointer to the array of UArticyOutputPin objects.
 */
const TArray<UArticyOutputPin*>* IArticyOutputPinsProvider::GetOutputPinsPtr() const
{
	const static auto name = FName("OutputPins");
	return Cast<IArticyReflectable>(this)->GetPropPtr<TArray<UArticyOutputPin*>>(name);
}

/**
 * @brief Provides the implementation for retrieving output pins.
 *
 * This function returns an array of output pins associated with the provider, or an empty array if none are found.
 *
 * @return An array of UArticyOutputPin objects.
 */
TArray<UArticyOutputPin*> IArticyOutputPinsProvider::GetOutputPins_Implementation() const
{
	auto Pins = GetOutputPinsPtr();
	if (Pins)
		return *Pins;
	else
		return TArray<UArticyOutputPin*>();
}
