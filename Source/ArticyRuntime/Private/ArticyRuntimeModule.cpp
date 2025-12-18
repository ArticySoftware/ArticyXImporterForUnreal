//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyRuntimeModule.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY(LogArticyRuntime)

/**
 * Called when the module is loaded into memory.
 * This is where you should initialize any resources or set up any state necessary for your module.
 * Currently, this method does not perform any actions.
 */
void FArticyRuntimeModule::StartupModule()
{
}

/**
 * Called when the module is unloaded from memory.
 * This is where you should clean up any resources or state that was initialized in StartupModule.
 * Currently, this method does not perform any actions.
 */
void FArticyRuntimeModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FArticyRuntimeModule, ArticyRuntime)
