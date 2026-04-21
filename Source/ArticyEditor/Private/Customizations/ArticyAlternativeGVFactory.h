//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "ArticyAlternativeGVFactory.generated.h"

/**
 * @class UArticyAlternativeGVFactory
 * @brief Factory class for creating UArticyAlternativeGlobalVariables assets.
 *
 * This class provides functionality for creating new instances of UArticyAlternativeGlobalVariables assets in the Unreal Engine editor.
 */
UCLASS(hidecategories = Object)
class UArticyAlternativeGVFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
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
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	/**
	 * @brief Determines whether the factory should appear in the "New" menu.
	 *
	 * @return True if the factory should be displayed in the "New" menu, false otherwise.
	 */
	virtual bool ShouldShowInNewMenu() const override;
};
