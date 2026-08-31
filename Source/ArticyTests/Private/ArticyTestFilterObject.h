//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithText.h"
#include "ArticyTestFilterObject.generated.h"

/**
 * @brief Stand-in for a generated object with an id, technical name, display name and text.
 *
 * Id and TechnicalName are only ever written during import, so tests need setters to build
 * a filterable object without imported content.
 */
UCLASS()
class UArticyTestFilterObject : public UArticyObject, public IArticyObjectWithDisplayName, public IArticyObjectWithText
{
	GENERATED_BODY()

public:

	void SetTestId(const FArticyId& InId) { Id = InId; }
	void SetTestTechnicalName(const FString& InName) { TechnicalName = InName; }

	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	FText Text;
};

/**
 * @brief Object with neither display name nor text, like a Jump.
 */
UCLASS()
class UArticyTestUnnamedObject : public UArticyObject
{
	GENERATED_BODY()

public:

	void SetTestId(const FArticyId& InId) { Id = InId; }
	void SetTestTechnicalName(const FString& InName) { TechnicalName = InName; }
};
