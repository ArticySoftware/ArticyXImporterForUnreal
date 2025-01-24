//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include <EdGraphUtilities.h>
#include "Slate/Pins/SArticyRefPin.h"
#include "Slate/Pins/SArticyIdPin.h"

/**
 * @class FArticyRefPinFactory
 * @brief A factory class for creating custom graph pins in the editor.
 *
 * This class extends FGraphPanelPinFactory to provide custom pins for Articy data types.
 * It checks the pin's type and creates the appropriate custom pin widget if the pin type
 * matches FArticyRef or FArticyId.
 */
class FArticyRefPinFactory final : public FGraphPanelPinFactory
{
public:

	/**
	 * @brief Creates a custom pin for the given graph pin if it matches specific types.
	 *
	 * This method overrides the CreatePin method from FGraphPanelPinFactory to produce
	 * custom pins for specific Articy data types. If the pin's sub-category object matches
	 * FArticyRef or FArticyId, a corresponding custom pin widget is created.
	 *
	 * @param Pin The graph pin for which a custom widget is to be created.
	 * @return A shared pointer to the created SGraphPin widget, or nullptr if no custom widget is created.
	 */
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override
	{
		if (Pin->PinType.PinSubCategoryObject.IsValid() && Pin->PinType.PinSubCategoryObject.Get() == FArticyRef::StaticStruct())
		{
			return SNew(SArticyRefPin, Pin);
		}
		if (Pin->PinType.PinSubCategoryObject.IsValid() && Pin->PinType.PinSubCategoryObject.Get() == FArticyId::StaticStruct())
		{
			return SNew(SArticyIdPin, Pin);
		}

		return nullptr;
	}
};
