//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "ArticyBaseTypes.h"
#include <SGraphPin.h>
#include "ArticyRef.h"

/**
 * A custom graph pin widget for Articy references.
 */
class SArticyRefPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SArticyRefPin) {}

	SLATE_END_ARGS()

	/**
	 * Constructs the SArticyRefPin widget.
	 *
	 * @param InArgs The arguments for construction.
	 * @param GraphPin The graph pin associated with this widget.
	 */
	void Construct(const FArguments& InArgs, UEdGraphPin* GraphPin);

	// SGraphPin Interface

	/**
	 * Gets the default value widget for the pin.
	 *
	 * @return A shared reference to the default value widget.
	 */
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

	/**
	 * Determines the visibility of the default value widget.
	 *
	 * @return The visibility state of the default value widget.
	 */
	virtual EVisibility GetDefaultValueVisibility() const override;

private:
	/** Serves as cache of the currently selected value.
	 * The actual value is saved in the DefaultValue of the parent UEdGraphPin
	 */
	FArticyRef ArticyRef = FArticyRef();

private:
	/**
	 * Retrieves the Articy reference from the pin's default value string.
	 *
	 * @return The Articy reference associated with this pin.
	 */
	FArticyRef GetArticyRef() const;

	/**
	 * Callback for when the Articy reference changes.
	 *
	 * @param NewArticyRef The new Articy reference value.
	 */
	void OnArticyRefChanged(const FArticyRef& NewArticyRef);
};
