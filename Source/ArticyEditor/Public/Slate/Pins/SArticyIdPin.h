//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "ArticyBaseTypes.h"
#include <SGraphPin.h>

/**
 * A custom graph pin widget for Articy IDs.
 */
class SArticyIdPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SArticyIdPin) {}

	SLATE_END_ARGS()

	/**
	 * Constructs the SArticyIdPin widget.
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
	FArticyId ArticyId = FArticyId();

private:
	/**
	 * Retrieves the Articy ID from the pin's default value string.
	 *
	 * @return The Articy ID associated with this pin.
	 */
	FArticyId GetArticyId() const;

	/**
	 * Callback for when the Articy ID changes.
	 *
	 * @param NewArticyId The new Articy ID value.
	 */
	void OnArticyIdChanged(const FArticyId& NewArticyId);
};
