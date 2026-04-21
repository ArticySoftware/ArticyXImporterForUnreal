//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArticyBaseTypes.h"
#include "Engine/Engine.h"
#include <Engine/World.h>

#include "ArticyType.h"
#include "Interfaces/ArticyReflectable.h"

#include "ArticyBaseObject.generated.h"

class UArticyPrimitive;

/**
 * Base class for all Articy objects.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyBaseObject : public UDataAsset, public IArticyReflectable
{
	GENERATED_BODY()

public:

	/** Returns true if deserialization was already performed for this object. */
	bool WasLoaded() const { return bWasDeserialized; }

	/** For ArticyImporter internal use only: initializes the bWasDeserialized variable. */
	void Initialize() { bWasDeserialized = true; }

	/**
	 * Retrieves a subobject of this Articy object using its unique identifier.
	 *
	 * @param Id The unique identifier of the subobject to retrieve.
	 * @return A pointer to the UArticyPrimitive subobject if found, otherwise nullptr.
	 */
	UArticyPrimitive* GetSubobject(FArticyId Id) const;

	/**
	 * Gets the Articy type of this object.
	 *
	 * @return The FArticyType associated with this object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy")
	FArticyType GetArticyType() const;

	/** The Articy type of this object. */
	FArticyType ArticyType;

protected:
	/**
	 * Initializes the object from a JSON value.
	 *
	 * @param Json The JSON value to initialize from.
	 */
	virtual void InitFromJson(TSharedPtr<FJsonValue> Json) { }

	template<typename Type, typename PropType>
	friend struct ArticyObjectTypeInfo;

	/**
	 * Adds a subobject to this Articy object.
	 *
	 * @param Obj The subobject to add.
	 */
	void AddSubobject(UArticyPrimitive* Obj);

	/** Objects that are dynamically created for this object during import (like Pins, Connections, etc.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TMap<FArticyId, UArticyPrimitive*> Subobjects;

	/**
	 * Retrieves a localized text property for this Articy object.
	 *
	 * @param Property The text property to localize.
	 * @return A localized FText object.
	 */
	UFUNCTION(BlueprintPure, Category = "Articy")
	FText GetPropertyText(const FText Property);

private:
	/** Initialized with false, changed to true by InitFromJson (and later by deserialization). */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	bool bWasDeserialized = false;
};

/** Base class for all Feature classes. */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyBaseFeature : public UArticyBaseObject
{
	GENERATED_BODY()
};
