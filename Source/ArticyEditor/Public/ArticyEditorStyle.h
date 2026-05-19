//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.   
//

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

/**
 * FArticyEditorStyle is a class responsible for managing the style and appearance of the Articy Editor
 * within Unreal Engine. It defines brushes, fonts, and styles for UI elements used in the plugin.
 */
class ARTICYEDITOR_API FArticyEditorStyle
{
public:

	/** Initialize the Articy editor style, creating and registering the Slate style set. */
	static void Initialize();

	/** Shutdown the Articy editor style, unregistering the Slate style set. */
	static void Shutdown();

	/** Reload textures used by the Slate renderer. */
	static void ReloadTextures();

	/**
	 * Get the Slate style instance for the Articy editor.
	 *
	 * @return The Slate style instance.
	 */
	static const ISlateStyle& Get();

	/**
	 * Get the name of the Slate style set.
	 *
	 * @return The name of the style set.
	 */
	static FName GetStyleSetName();

private:

	/**
	 * Create the Slate style set, defining brushes, fonts, and styles for UI elements.
	 *
	 * @return The created style set.
	 */
	static TSharedRef< class FSlateStyleSet > Create();

private:

	/** Shared pointer to the Slate style set instance. */
	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
