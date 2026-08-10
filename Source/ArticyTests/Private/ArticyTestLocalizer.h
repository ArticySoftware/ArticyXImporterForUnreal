//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyLocalizerSystem.h"
#include "ArticyTestLocalizer.generated.h"

/**
 * @brief Stand-in for a generated localizer whose loaded state tests can set.
 *
 * The real localizer only exists in a project with imported content, and bDataLoaded is
 * flipped by its generated Reload(). Tests need both states to cover the string table
 * lookup path, so this exposes the flag without any generated content.
 */
UCLASS()
class UArticyTestLocalizer : public UArticyLocalizerSystem
{
	GENERATED_BODY()

public:

	/** Pretends the articy string table has (or has not) been loaded. */
	void SetDataLoaded(bool bLoaded) { bDataLoaded = bLoaded; }
};
