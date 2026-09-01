//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithText.h"
#include "ArticyTestTextModel.generated.h"

/**
 * @brief Stand-in for a generated class with one property of each articy text kind.
 *
 * The predefined-type deserializers write through SetProp by name, so they need a reflected
 * object carrying those properties.
 */
UCLASS()
class UArticyTestTextModel : public UArticyObject
{
	GENERATED_BODY()

public:

	/** Receives an ArticyString. */
	UPROPERTY()
	FString Plain;

	/** Receives an ArticyMultiLanguageString. */
	UPROPERTY()
	FText Localized;
};

/**
 * @brief Stand-in for a generated Hub, whose DisplayName and Text are plain ArticyStrings.
 */
UCLASS()
class UArticyTestPlainNamedObject : public UArticyObject, public IArticyObjectWithDisplayName, public IArticyObjectWithText
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FString Text;
};
