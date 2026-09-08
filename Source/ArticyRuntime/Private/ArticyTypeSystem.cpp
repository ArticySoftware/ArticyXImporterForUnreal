//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyTypeSystem.h"

#include "ArticyDatabase.h"
#include "ArticyRuntimeModule.h"
#include "ArticyType.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif

namespace
{
	// The importer fills the type map on a generated <Project>TypeSystem asset that derives
	// from UArticyTypeSystem; the base class itself is never saved as an asset.
	UArticyTypeSystem* FindGeneratedTypeSystem()
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		AssetRegistryModule.Get().GetAssetsByClass(UArticyTypeSystem::StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(UArticyTypeSystem::StaticClass()->GetFName(), AssetData, true);
#endif

		if (AssetData.Num() > 1)
		{
			UE_LOG(LogArticyRuntime, Warning, TEXT("More than one ArticyTypeSystem was found, this is not supported! The first one will be selected."));
		}

		for (const FAssetData& Asset : AssetData)
		{
			if (UArticyTypeSystem* TypeSystem = Cast<UArticyTypeSystem>(Asset.GetAsset()))
			{
				return TypeSystem;
			}
		}

		return nullptr;
	}
}

UArticyTypeSystem* UArticyTypeSystem::Get()
{
	static TWeakObjectPtr<UArticyTypeSystem> ArticyTypeSystem;
	static bool bIsGenerated = false;

	if (!ArticyTypeSystem.IsValid())
	{
		bIsGenerated = false;
	}

	// Keep looking while we only hold the empty placeholder: the generated asset does not
	// exist before the first import, and the asset registry may still be scanning.
	if (!bIsGenerated)
	{
		if (UArticyTypeSystem* Generated = FindGeneratedTypeSystem())
		{
			ArticyTypeSystem = Generated;
			bIsGenerated = true;
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
	if (const FArticyType* Type = Types.Find(TypeName))
	{
		return *Type;
	}

	return {};
}
