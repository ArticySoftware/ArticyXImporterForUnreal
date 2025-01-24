//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Customizations/ArticyEditorCustomizationManager.h"
#include "Input/Reply.h"

/**
 * @class FArticyButtonCustomization
 * @brief Customizes the Articy ID property widget with an additional button.
 *
 * This class adds a custom button to the Articy ID property widget, providing additional functionality for interacting with Articy objects.
 */
class FArticyButtonCustomization final : public IArticyIdPropertyWidgetCustomization
{
public:
	/**
	 * @brief Registers a custom widget for Articy ID property with an additional toolbar button.
	 *
	 * Creates an extra button in the toolbar to interact with Articy objects.
	 *
	 * @param Builder A builder object for constructing Articy ID property widget customizations.
	 */
	virtual void RegisterArticyIdPropertyWidgetCustomization(FArticyIdPropertyWidgetCustomizationBuilder& Builder) override;

	/**
	 * @brief Unregisters the custom widget for Articy ID property.
	 *
	 * Clears the reference to the Articy object when the customization is unregistered.
	 */
	virtual void UnregisterArticyIdPropertyWidgetCustomization() override;

	/**
	 * @brief Creates a button for the Articy toolbar.
	 *
	 * Adds a button to the toolbar, allowing users to interact with Articy objects.
	 *
	 * @param Builder A toolbar builder for constructing toolbar elements.
	 */
	void CreateArticyButton(FToolBarBuilder& Builder);

	/**
	 * @brief Handles the click event for the Articy button.
	 *
	 * Called when the Articy button is clicked, shows the selected object in Articy Draft.
	 *
	 * @return A reply indicating that the button click was handled.
	 */
	FReply OnArticyButtonClicked();

private:
	/** A weak pointer to the Articy object associated with this customization. */
	TWeakObjectPtr<const UArticyObject> ArticyObject = nullptr;
};

/**
 * @class FArticyButtonCustomizationFactory
 * @brief Factory for creating instances of Articy button customizations.
 *
 * This class is responsible for creating and managing instances of Articy button customizations.
 */
class FArticyButtonCustomizationFactory final : public IArticyIdPropertyWidgetCustomizationFactory
{
public:
	/**
	 * @brief Creates an instance of the Articy button customization.
	 *
	 * Factory method that creates and returns a new instance of the Articy button customization.
	 *
	 * @return A shared pointer to the created customization instance.
	 */
	virtual TSharedPtr<IArticyIdPropertyWidgetCustomization> CreateCustomization() override;

	/**
	 * @brief Checks if the customization supports the given Articy object type.
	 *
	 * Determines whether the customization can be applied to the specified Articy object type.
	 *
	 * @param ArticyObject The Articy object to check.
	 * @return True if the customization supports the object type, otherwise false.
	 */
	virtual bool SupportsType(const UArticyObject* ArticyObject) override;
};
