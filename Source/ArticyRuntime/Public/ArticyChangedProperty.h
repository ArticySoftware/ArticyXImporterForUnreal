//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyChangedProperty.generated.h"

class UArticyBaseObject;
class IArticyReflectable;

/** @brief A property change on an articy object, as passed to its listeners. */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyChangedProperty
{
	GENERATED_BODY()

public:
	void SetObjectReference(IArticyReflectable* object);
	
	/** The object whose property changed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	UArticyBaseObject* ObjectReference = nullptr;

	/** The name of the changed property, `Feature.Property` for a template property. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FName Property = NAME_None;
};
