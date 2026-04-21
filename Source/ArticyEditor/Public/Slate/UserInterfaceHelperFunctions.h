//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "ArticyBaseTypes.h"
#include "Styling/SlateBrush.h"

class UArticyObject;

/**
 * A namespace containing utility functions for handling user interface elements related to Articy objects.
 */
namespace UserInterfaceHelperFunctions
{
	/**
	 * Enumeration representing the possible sizes for images.
	 */
	enum EImageSize
	{
		Small,
		Medium,
		Large
	};

	/**
	 * Retrieves the type image associated with an Articy object based on its type and specified size.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the type image.
	 * @param Size The desired size of the image (Small, Medium, or Large).
	 * @return The Slate brush representing the type image for the specified Articy object.
	 */
	const ARTICYEDITOR_API FSlateBrush* GetArticyTypeImage(const UArticyObject* ArticyObject, EImageSize Size);

	/**
	 * Retrieves the display image (preview image) associated with an Articy object.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the display image.
	 * @return The texture representing the display image for the specified Articy object.
	 */
	ARTICYEDITOR_API UTexture2D* GetDisplayImage(const UArticyObject* ArticyObject);

	/**
	 * Retrieves the preview image associated with an Articy object and assigns it to the specified Slate brush.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the preview image.
	 * @param OutSlateBrush The Slate brush to which the preview image will be assigned.
	 * @return True if a preview image was successfully retrieved and assigned; otherwise, false.
	 */
	ARTICYEDITOR_API bool RetrievePreviewImage(const UArticyObject* ArticyObject, FSlateBrush& OutSlateBrush);

	/**
	 * Retrieves the preview image associated with the speaker of an Articy object and assigns it to the specified Slate brush.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the speaker's preview image.
	 * @param OutSlateBrush The Slate brush to which the speaker's preview image will be assigned.
	 * @return True if a speaker's preview image was successfully retrieved and assigned; otherwise, false.
	 */
	ARTICYEDITOR_API bool RetrieveSpeakerPreviewImage(const UArticyObject* ArticyObject, FSlateBrush& OutSlateBrush);

	/**
	 * Retrieves the target ID associated with an Articy object.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the target ID.
	 * @return A pointer to the target ID associated with the specified Articy object.
	 */
	const ARTICYEDITOR_API FArticyId* GetTargetID(const UArticyObject* ArticyObject);

	/**
	 * Retrieves the display name associated with an Articy object.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the display name.
	 * @return The string representing the display name for the specified Articy object.
	 */
	const ARTICYEDITOR_API FString GetDisplayName(const UArticyObject* ArticyObject);

	/**
	 * Retrieves the color associated with an Articy object.
	 *
	 * @param ArticyObject The Articy object for which to retrieve the color.
	 * @return The linear color associated with the specified Articy object.
	 */
	const ARTICYEDITOR_API FLinearColor GetColor(const UArticyObject* ArticyObject);

	/**
	 * Displays an Articy object in the Articy software.
	 *
	 * @param ArticyObject The Articy object to be displayed.
	 * @return True if the Articy object was successfully displayed; otherwise, false.
	 */
	const ARTICYEDITOR_API bool ShowObjectInArticy(const UArticyObject* ArticyObject);

	/**
	 * Displays an Articy object with the specified ID in the Articy software.
	 *
	 * @param ArticyId The ID of the Articy object to be displayed.
	 * @param bNewTab Specifies whether to open the object in a new tab (true) or the current tab (false).
	 * @return True if the Articy object was successfully displayed; otherwise, false.
	 */
	const ARTICYEDITOR_API bool ShowObjectInArticy(const FArticyId ArticyId, bool bNewTab);
};
