//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyType.h"
#include "ArticyTypeSystem.generated.h"

UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyTypeSystem : public UObject
{
	GENERATED_BODY()

public:
	static UArticyTypeSystem* Get();
	FArticyType GetArticyType(const FString& TypeName) const;
	TMap<FString, FArticyType> Types;
};
