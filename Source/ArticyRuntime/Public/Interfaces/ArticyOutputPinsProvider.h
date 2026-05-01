//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPins.h"
#include "ArticyFlowObject.h"
#include "ArticyOutputPinsProvider.generated.h"

/**
 * @interface UArticyOutputPinsProvider
 * @brief Interface for objects with output pins.
 *
 * All objects that have a property called 'OutputPins' (an array of UArticyInputPin*)
 * implement this interface.
 */
UINTERFACE()
class ARTICYRUNTIME_API UArticyOutputPinsProvider : public UArticyFlowObject { GENERATED_BODY() };

/**
 * @class IArticyOutputPinsProvider
 * @brief Interface class for providing output pins functionality.
 *
 * This interface class provides the functionality to manage and explore output pins
 * within a flow system.
 */
class ARTICYRUNTIME_API IArticyOutputPinsProvider : public IArticyFlowObject
{
	GENERATED_BODY()

public:

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
	void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override;

	/**
	 * @brief Retrieves the output pins of the provider.
	 *
	 * This function returns a pointer to the array of output pins associated with the provider.
	 *
	 * @return A pointer to the array of UArticyOutputPin objects.
	 */
	const TArray<UArticyOutputPin*>* GetOutputPinsPtr() const;

	/**
	 * @brief Blueprint callable event to retrieve output pins.
	 *
	 * This event provides the blueprint functionality to retrieve output pins.
	 *
	 * @return An array of UArticyOutputPin objects.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Articy")
	TArray<UArticyOutputPin*> GetOutputPins() const;

	/**
	 * @brief Provides the implementation for retrieving output pins.
	 *
	 * This function returns an array of output pins associated with the provider, or an empty array if none are found.
	 *
	 * @return An array of UArticyOutputPin objects.
	 */
	TArray<UArticyOutputPin*> GetOutputPins_Implementation() const;
};
