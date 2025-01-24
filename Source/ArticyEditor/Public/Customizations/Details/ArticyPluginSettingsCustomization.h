//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "IDetailCustomization.h"

/**
 * @class FArticyPluginSettingsCustomization
 * @brief Customizes the details panel for Articy plugin settings in Unreal Engine's editor.
 *
 * This class provides custom UI elements and functionality for editing Articy plugin settings, including dynamic updating of the UI after asset generation.
 */
class FArticyPluginSettingsCustomization : public IDetailCustomization
{
public:
	/**
	 * @brief Default constructor for FArticyPluginSettingsCustomization.
	 */
	FArticyPluginSettingsCustomization();

	/**
	 * @brief Destructor for FArticyPluginSettingsCustomization.
	 */
	virtual ~FArticyPluginSettingsCustomization() override;

	/**
	 * @brief Creates a shared instance of FArticyPluginSettingsCustomization.
	 *
	 * @return A shared pointer to a new instance of FArticyPluginSettingsCustomization.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/**
	 * @brief Customizes the details panel layout for Articy plugin settings.
	 *
	 * This function sets up the custom UI for managing Articy package settings.
	 *
	 * @param DetailLayout The detail layout builder used for customizing the details panel.
	 */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	/**
	 * @brief Refreshes the settings UI by forcing a refresh of the detail layout.
	 *
	 * This function is called after assets are generated or files are loaded to update the UI.
	 */
	void RefreshSettingsUI();

	/** Handle for the refresh delegate. */
	FDelegateHandle RefreshHandle;

	/** Pointer to the detail layout builder. */
	IDetailLayoutBuilder* LayoutBuilder = nullptr;
};
