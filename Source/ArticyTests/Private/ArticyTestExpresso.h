//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "ArticyExpressoScripts.h"
#include "ArticyTestExpresso.generated.h"

/**
 * @brief Minimal concrete expresso scripts class.
 *
 * UArticyExpressoScripts is abstract; tests that only need an instance to exist (for lifetime
 * or caching checks) create one of these instead of depending on a generated project class.
 */
UCLASS()
class UArticyTestExpresso : public UArticyExpressoScripts
{
	GENERATED_BODY()
};
