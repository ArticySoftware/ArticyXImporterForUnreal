//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyChangedProperty.generated.h"

class UArticyBaseObject;

USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyChangedProperty
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	UArticyBaseObject* ObjectReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FName Property;
};
