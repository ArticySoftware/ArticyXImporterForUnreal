//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyEditorCommands.h"

#define LOCTEXT_NAMESPACE "FArticyImporterModule"

/**
 * Registers the commands used by the Articy Editor plugin.
 * This method is called to define and bind the commands to the actions they perform.
 */
void FArticyEditorCommands::RegisterCommands()
{
	// FInputChord can be modified for default shortcuts, but they don't seem to work inside the level editor by default
	UI_COMMAND(OpenArticyImporter, "Articy X Importer", "Bring up ArticyImporter window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenArticyGvDebugger, "Articy GV Debugger", "Bring up the runtime Global Variables debugger", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
