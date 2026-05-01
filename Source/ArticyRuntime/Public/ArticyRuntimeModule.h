//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogArticyRuntime, Log, All)

/**
 * Module for the Articy Runtime.
 *
 * This module handles the initialization and shutdown of the Articy runtime, and is responsible for
 * setting up any necessary resources or state when loaded, and cleaning up when unloaded.
 */
class FArticyRuntimeModule : public IModuleInterface
{
public:

	/**
	 * Initializes the module.
	 * This method is called when the module is loaded into memory.
	 */
	virtual void StartupModule() override;

	/**
	 * Cleans up the module.
	 * This method is called when the module is unloaded from memory.
	 */
	virtual void ShutdownModule() override;
};
