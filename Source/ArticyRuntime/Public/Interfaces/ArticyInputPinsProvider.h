//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPins.h"
#include "ArticyFlowObject.h"
#include "ArticyInputPinsProvider.generated.h"

/**
 * @class UArticyInputPinsProvider
 * @brief Interface for objects with input pins.
 *
 * This interface is implemented by all objects that have a property called 'InputPins',
 * which is an array of UArticyInputPin pointers. It provides functionality for interacting
 * with these input pins.
 */
UINTERFACE()
class ARTICYRUNTIME_API UArticyInputPinsProvider : public UArticyFlowObject { GENERATED_BODY() };

/**
 * @class IArticyInputPinsProvider
 * @brief Interface class for objects providing input pins.
 *
 * This interface class defines methods for interacting with input pins,
 * allowing for exploration and manipulation of flow nodes in Articy.
 */
class ARTICYRUNTIME_API IArticyInputPinsProvider : public IArticyFlowObject
{
	GENERATED_BODY()

public:

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
	bool TrySubmerge(class UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth, const bool bForceShadowed);

	/**
	 * @brief Retrieves a pointer to the InputPins array.
	 *
	 * This function returns a pointer to the array of input pins, if available, for this object.
	 *
	 * @return A pointer to an array of UArticyInputPin objects, or nullptr if not found.
	 */
	const TArray<UArticyInputPin*>* GetInputPinsPtr() const;

	/**
	 * @brief Retrieves the input pins for this object.
	 *
	 * This function provides a BlueprintCallable and BlueprintNativeEvent version
	 * of the GetInputPins functionality, allowing for easy access to input pins.
	 *
	 * @return An array of UArticyInputPin objects.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Articy")
	TArray<UArticyInputPin*> GetInputPins() const;

	/**
	 * @brief Implementation of the GetInputPins function.
	 *
	 * This function provides the actual implementation for retrieving the array of input pins.
	 *
	 * @return An array of UArticyInputPin objects.
	 */
	TArray<UArticyInputPin*> GetInputPins_Implementation() const;
};
