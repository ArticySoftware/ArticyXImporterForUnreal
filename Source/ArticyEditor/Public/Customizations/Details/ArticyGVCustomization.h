//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * @class FArticyGVCustomization
 * @brief Customizes the details panel for Articy Global Variables.
 *
 * This class provides customization for the details panel of ArticyGlobalVariables objects, hiding the default categories and replacing them with a custom UI.
 */
class FArticyGVCustomization : public IDetailCustomization
{
public:
	/**
	 * @brief Creates an instance of the ArticyGVCustomization.
	 *
	 * This function creates and returns a shared pointer to a new instance of the ArticyGVCustomization class.
	 *
	 * @return A shared pointer to a new instance of the ArticyGVCustomization class.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/**
	 * @brief Customizes the details panel for Articy Global Variables.
	 *
	 * This function customizes the details panel for a single ArticyGlobalVariables object, hiding the default categories and displaying a custom UI.
	 *
	 * @param DetailBuilder The detail layout builder used to customize the details panel.
	 */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
