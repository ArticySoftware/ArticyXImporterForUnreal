//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyAlternativeGVFactory.h"
#include "ArticyAlternativeGlobalVariables.h"

/**
 * @brief Constructs a new instance of UArticyAlternativeGVFactory.
 *
 * This constructor initializes the factory settings for creating new UArticyAlternativeGlobalVariables objects.
 *
 * @param ObjectInitializer The object initializer for the factory.
 */
UArticyAlternativeGVFactory::UArticyAlternativeGVFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UArticyAlternativeGlobalVariables::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

/**
 * @brief Creates a new UArticyAlternativeGlobalVariables object.
 *
 * This function is called when a new instance of the asset is created in the editor.
 *
 * @param InClass The class type of the object to create.
 * @param InParent The parent object for the new asset.
 * @param InName The name of the new asset.
 * @param Flags Object flags for the new asset.
 * @param Context Additional context for asset creation.
 * @param Warn Feedback context for warnings.
 * @return A pointer to the newly created UArticyAlternativeGlobalVariables object.
 */
UObject* UArticyAlternativeGVFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UArticyAlternativeGlobalVariables>(InParent, InClass, InName, Flags);
}

/**
 * @brief Determines whether the factory should appear in the "New" menu.
 *
 * @return True if the factory should be displayed in the "New" menu, false otherwise.
 */
bool UArticyAlternativeGVFactory::ShouldShowInNewMenu() const
{
	return true;
}
