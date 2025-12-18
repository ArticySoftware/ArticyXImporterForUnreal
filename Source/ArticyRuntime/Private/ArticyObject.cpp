//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyObject.h"
#include "ArticyHelpers.h"
#include "ArticyDatabase.h"
#include "ArticyBaseTypes.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0 
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "ArticyPackage.h"

#if WITH_EDITOR
TSet<TWeakObjectPtr<UArticyPackage>> UArticyObject::CachedPackages;
TMap<FArticyId, TWeakObjectPtr<UArticyObject>> UArticyObject::ArticyIdCache;
TMap<FName, TWeakObjectPtr<UArticyObject>> UArticyObject::ArticyNameCache;
#endif

/**
 * Initializes the UArticyObject from a JSON value.
 *
 * @param Json The JSON value to initialize from. Must be a JSON object.
 */
void UArticyObject::InitFromJson(TSharedPtr<FJsonValue> Json)
{
	Super::InitFromJson(Json);

	if (!Json.IsValid() || Json->Type != EJson::Object)
		return;

	auto obj = Json->AsObject();
	if (!ensure(obj.IsValid()))
		return;

	JSON_TRY_HEX_ID(obj, Parent);
	JSON_TRY_STRING(obj, TechnicalName);
}

//---------------------------------------------------------------------------//

/**
 * Gets the technical name of the Articy object.
 *
 * @return The technical name of the object.
 */
FName UArticyObject::GetTechnicalName() const
{
	return *TechnicalName;
}

/**
 * Gets the parent Articy object.
 *
 * @return Pointer to the parent Articy object, or nullptr if not found.
 */
UArticyObject* UArticyObject::GetParent() const
{
	return UArticyDatabase::Get(this)->GetObject<UArticyObject>(Parent);
}

/**
 * Gets the children of this Articy object.
 *
 * @return An array of weak pointers to the child Articy objects.
 */
TArray<TWeakObjectPtr<UArticyObject>> UArticyObject::GetChildren() const
{
	if (CachedChildren.Num() != Children.Num())
	{
		auto db = UArticyDatabase::Get(this);
		CachedChildren.Empty(Children.Num());

		for (auto childId : Children)
			if (auto child = db->GetObject<UArticyObject>(childId))
				CachedChildren.Add(child);
	}

	return CachedChildren;
}

/**
 * Gets the ID of the parent Articy object.
 *
 * @return The ID of the parent Articy object.
 */
FArticyId UArticyObject::GetParentID() const
{
	return Parent;
}

/**
 * Gets the IDs of the children Articy objects.
 *
 * @return An array of IDs for the children Articy objects.
 */
TArray<FArticyId> UArticyObject::GetChildrenIDs() const
{
	return Children;
}

#if WITH_EDITOR

/**
 * Gets the IDs of all children Articy objects that map to actual articy objects (excluding pins, etc.).
 *
 * @return An array of IDs for the articy object children.
 */
TArray<FArticyId> UArticyObject::GetArticyObjectChildrenIDs() const
{
	TArray<FArticyId> OutIDs;

	for (auto ChildID : Children)
	{
		if (const UArticyObject* Object = UArticyObject::FindAsset(ChildID))
		{
			OutIDs.Add(Object->GetId());
		}
	}

	return OutIDs;
}

/**
 * Finds an Articy object by its ID. Caches and refreshes packages if necessary.
 *
 * @param Id The ID of the Articy object to find.
 * @return Pointer to the Articy object if found, or nullptr if not found.
 */
UArticyObject* UArticyObject::FindAsset(const FArticyId& Id)
{
	if (ArticyIdCache.Contains(Id) && ArticyIdCache[Id].IsValid())
	{
		return ArticyIdCache[Id].Get();
	}

	bool bRefreshPackages = false;

	if (CachedPackages.Num() >= 1)
	{
		for (auto& Package : CachedPackages)
		{
			bRefreshPackages = !Package.IsValid();

			if (bRefreshPackages)
			{
				break;
			}
		}
	}
	else if (CachedPackages.Num() == 0)
	{
		bRefreshPackages = true;
	}

	// refresh packages if needed 
	if (bRefreshPackages)
	{
		CachedPackages.Empty();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetFName(), AssetData, true);
#endif

		for (const auto& ArticyPackage : AssetData)
		{
			const auto& Package = Cast<UArticyPackage>(ArticyPackage.GetAsset());

			if (Package != nullptr)
			{
				CachedPackages.Add(Package);
			}
		}
	}

	for (TWeakObjectPtr<UArticyPackage> ArticyPackage : CachedPackages)
	{
		if (ArticyPackage.IsValid())
		{
			UArticyObject* ArticyObject = ArticyPackage->GetAssetById(Id);
			if (ArticyObject && ArticyObject->WasLoaded())
			{
				ArticyIdCache.Add(Id, ArticyObject);
				ArticyNameCache.Add(ArticyObject->GetTechnicalName(), ArticyObject);
				return ArticyObject;
			}
		}
	}

	// if the object wasn't found in any package but exists in the object map, remove it
	if (ArticyIdCache.Contains(Id))
	{
		ArticyIdCache.Remove(Id);
	}

	return nullptr;
}

/**
 * Finds an Articy object by its technical name. Caches and refreshes packages if necessary.
 *
 * @param TechnicalName The technical name of the Articy object to find.
 * @return Pointer to the Articy object if found, or nullptr if not found.
 */
UArticyObject* UArticyObject::FindAsset(const FString& TechnicalName)// MM_CHANGE
{
	const FName Name(*TechnicalName);
	if (ArticyNameCache.Contains(Name) && ArticyNameCache[Name].IsValid())
	{
		return ArticyNameCache[Name].Get();
	}

	bool bRefreshPackages = false;

	if (CachedPackages.Num() >= 1)
	{
		for (auto& Package : CachedPackages)
		{
			bRefreshPackages = !Package.IsValid();

			if (bRefreshPackages)
			{
				break;
			}
		}
	}
	else if (CachedPackages.Num() == 0)
	{
		bRefreshPackages = true;
	}

	// refresh packages if needed 
	if (bRefreshPackages)
	{
		CachedPackages.Empty();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetFName(), AssetData, true);
#endif
		for (const auto& ArticyPackage : AssetData)
		{
			const auto& Package = Cast<UArticyPackage>(ArticyPackage.GetAsset());

			if (Package != nullptr)
			{
				CachedPackages.Add(Package);
			}
		}
	}

	for (TWeakObjectPtr<UArticyPackage> ArticyPackage : CachedPackages)
	{
		if (ArticyPackage.IsValid())
		{
			UArticyObject* ArticyObject = ArticyPackage->GetAssetByTechnicalName(Name);
			if (ArticyObject && ArticyObject->WasLoaded())
			{
				ArticyNameCache.Add(Name, ArticyObject);
				ArticyIdCache.Add(ArticyObject->GetId(), ArticyObject);
				return ArticyObject;
			}
		}
	}

	// if the object wasn't found in any package but exists in the object map, remove it
	if (ArticyNameCache.Contains(Name))
	{
		ArticyNameCache.Remove(Name);
	}

	return nullptr;
}
#endif
