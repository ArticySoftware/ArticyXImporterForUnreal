//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyTypeSystem.h"
#include "ArticyHelpers.h"
#include "Runtime/Launch/Resources/Version.h"

#include "ArticyDatabase.h"
#include "ArticyType.h"

UArticyTypeSystem* UArticyTypeSystem::Get()
{
	static TWeakObjectPtr<UArticyTypeSystem> ArticyTypeSystem;

	if (!ArticyTypeSystem.IsValid())
	{
		// The import generates a type system asset next to the database; prefer that one, since
		// a fresh object knows no types at all.
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetFName(), AssetData, true);
#endif
		for (const FAssetData& Asset : AssetData)
		{
			if (UArticyTypeSystem* Loaded = Cast<UArticyTypeSystem>(Asset.GetAsset()))
			{
				ArticyTypeSystem = Loaded;
				break;
			}
		}
	}

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
