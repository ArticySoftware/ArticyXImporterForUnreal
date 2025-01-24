//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyEditorStyle.h"
#include "Framework/Commands/Commands.h"

/**
 * FArticyEditorCommands is responsible for registering and managing commands
 * used by the Articy Editor plugin.
 */
class FArticyEditorCommands : public TCommands<FArticyEditorCommands>
{
public:

	/**
	 * Constructor for FArticyEditorCommands.
	 * Initializes the command set with context and style information.
	 */
	FArticyEditorCommands()
		: TCommands(TEXT("ArticyImporter"), NSLOCTEXT("Contexts", "ArticyImporter", "ArticyImporter Plugin"), NAME_None, FArticyEditorStyle::GetStyleSetName())
	{
	}

	/** Registers the commands used by this command set. */
	virtual void RegisterCommands() override;

	/** Command for opening the Articy Importer window. */
	TSharedPtr<FUICommandInfo> OpenArticyImporter;

	/** Command for opening the Articy Global Variables Debugger. */
	TSharedPtr<FUICommandInfo> OpenArticyGvDebugger;
};
