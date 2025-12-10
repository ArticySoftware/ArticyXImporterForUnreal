//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyChangedProperty.generated.h"

class UArticyBaseObject;
class IArticyReflectable;

USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyChangedProperty
{
	GENERATED_BODY()

public:
	void SetObjectReference(IArticyReflectable* object);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	UArticyBaseObject* ObjectReference = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FName Property = NAME_None;
};
