//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyTypeSystem.h"

#include "ArticyDatabase.h"
#include "ArticyType.h"

UArticyTypeSystem* UArticyTypeSystem::Get()
{
	static TWeakObjectPtr<UArticyTypeSystem> ArticyTypeSystem;

	if (!ArticyTypeSystem.IsValid())
	{
		ArticyTypeSystem = TWeakObjectPtr<UArticyTypeSystem>(NewObject<UArticyTypeSystem>());
	}

	return ArticyTypeSystem.Get();
}

FArticyType UArticyTypeSystem::GetArticyType(const FString& TypeName) const
{
	if (Types.Contains(TypeName))
	{
		return Types[TypeName];
	}

	return {};
}
