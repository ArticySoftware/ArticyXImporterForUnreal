//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyEditorFunctionLibrary.h"
#include "ArticyImportData.h"

#define LOCTEXT_NAMESPACE "ArticyImporter"

// Reference: See AssetRegistryConsoleCommands.h

class FArticyEditorModule;

/**
 * @class FArticyEditorConsoleCommands
 * @brief Provides console commands for the Articy Editor module in Unreal Engine.
 *
 * This class defines and implements console commands for the Articy Editor module, allowing
 * users to interact with and reimport Articy data into Unreal Engine through the console.
 */
class FArticyEditorConsoleCommands
{
public:

	/**
	 * @brief Constructor that initializes the console commands for the given ArticyEditorModule instance.
	 *
	 * @param InModule The ArticyEditorModule instance for which console commands are defined.
	 */
	explicit FArticyEditorConsoleCommands(const FArticyEditorModule& InModule)
		: Module(InModule)

		, ReimportCommand(
			TEXT("ArticyImporter.Reimport"),
			*LOCTEXT("CommandText_Reimport", "Reimport articy data into Unreal").ToString(),
			FConsoleCommandDelegate::CreateRaw(this, &FArticyEditorConsoleCommands::Reimport))

	{}

	/**
	 * @brief Reimports Articy data into Unreal Engine.
	 *
	 * This method is bound to a console command and performs the reimport operation,
	 * loading Articy data into the engine and updating the corresponding assets.
	 */
	void Reimport()
	{
		UArticyImportData* ImportData = nullptr;
		FArticyEditorFunctionLibrary::EnsureImportDataAsset(&ImportData);

		if (ImportData)
		{
			if (const auto& Factory = NewObject<UArticyJSONFactory>())
			{
				Factory->Reimport(ImportData);
				//GC will destroy factory
			}
		}
	}

private:

	/** Reference to the ArticyEditorModule instance associated with these console commands. */
	const FArticyEditorModule& Module;

	/** Console command for reimporting Articy data. */
	FAutoConsoleCommand ReimportCommand;
};

#undef LOCTEXT_NAMESPACE
