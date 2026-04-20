//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyDatabase.h"
#include "Engine/GameInstance.h"
#include "ArticyBaseTypes.h"
#include "ArticyGlobalVariables.h"
#include "ArticyPluginSettings.h"
#include "ArticyExpressoScripts.h"
#include "Misc/Paths.h"

/**
 * Gets the underlying Articy object.
 * @return A pointer to the underlying UArticyObject.
 */
UArticyObject* FArticyObjectShadow::GetObject()
{
	return Object;
}

/**
 * Constructs a shadowable object, initializing it with the provided object.
 * @param Object The Articy object to shadow.
 * @param CloneId The clone ID for the object.
 * @param Outer The outer object context for the shadowable object.
 */
FArticyShadowableObject::FArticyShadowableObject(UArticyObject* Object, int32 CloneId, UObject* Outer)
{
	Object->SetCloneID(CloneId);
	ShadowCopies.Add(FArticyObjectShadow(0, Object, CloneId, Outer));
}

/**
 * Retrieves the shadowed Articy object based on the shadow manager and clone state.
 * @param ShadowManager The manager for shadow states.
 * @param bForceUnshadowed Force retrieval of the unshadowed version.
 * @return A pointer to the shadowed or unshadowed UArticyObject.
 */
UArticyObject* FArticyShadowableObject::Get(const IShadowStateManager* ShadowManager, bool bForceUnshadowed) const
{
	if (bForceUnshadowed)
		return ShadowCopies[0].GetObject();

	const auto& ShadowLvl = ShadowManager->GetShadowLevel();
	FArticyObjectShadow* info = ShadowCopies.FindByPredicate([&](const FArticyObjectShadow& item)
		{
			return item.ShadowLevel == ShadowLvl;
		});

	if (info)
		return info->GetObject();

	if (!ensureMsgf(ShadowLvl > 0 && ShadowCopies.Num() > 0, TEXT("Cannot get shadow level %d of FArticyShadowableObject!"), ShadowLvl))
		return nullptr;

	//create a new shadow copy
	auto& mostRecentShadow = ShadowCopies.Last();
	auto SourceObject = mostRecentShadow.GetObject();
	auto obj = DuplicateObject(SourceObject, SourceObject);
	ShadowCopies.Add(FArticyObjectShadow(ShadowLvl, obj, mostRecentShadow.GetCloneId()));

#if __cplusplus >= 202002L
	const_cast<IShadowStateManager*>(ShadowManager)->RegisterOnPopState([=, this]
#else
	const_cast<IShadowStateManager*>(ShadowManager)->RegisterOnPopState([=]
#endif
		{
			//when the state is popped, remove the shadow copy again
			//it is destroyed automatically, unless there is an owning reference to it
			ensure(ShadowCopies.Last().ShadowLevel == ShadowLvl);
			ShadowCopies.RemoveAt(ShadowCopies.Num() - 1);
		});

	//return the new shadow copy
	return obj;
}

/**
 * Retrieves a clone of the Articy object based on clone ID and shadow state.
 * @param ShadowManager The manager for shadow states.
 * @param CloneId The ID of the clone to retrieve.
 * @param bForceUnshadowed Force retrieval of the unshadowed version.
 * @return A pointer to the shadowed or unshadowed UArticyObject clone.
 */
UArticyObject* UArticyCloneableObject::Get(const IShadowStateManager* ShadowManager, int32 CloneId,
	bool bForceUnshadowed) const
{
	auto info = Clones.Find(CloneId);
	return info ? info->Get(ShadowManager, bForceUnshadowed) : nullptr;
}

/**
 * Clones the Articy object, creating a new instance with the specified clone ID.
 * @param ShadowManager The manager for shadow states.
 * @param CloneId The ID for the new clone.
 * @param bFailIfExists Fails if the clone already exists.
 * @return A pointer to the newly created or existing clone, or nullptr if creation failed.
 */
UArticyObject* UArticyCloneableObject::Clone(const IShadowStateManager* ShadowManager, int32 CloneId,
	bool bFailIfExists)
{
	auto clone = Get(ShadowManager, CloneId);

	if (bFailIfExists && clone)
		return nullptr;

	if (!clone)
	{
		//get the original object (clone 0)
		auto original = Get(ShadowManager);
		if (ensure(original))
		{
			//create the clone
			clone = DuplicateObject(original, original);
			AddClone(clone, CloneId);
		}
	}

	return clone;
}

/**
 * Adds a clone to the clone map with a specified clone ID.
 * @param Clone The clone to add.
 * @param CloneId The ID for the clone.
 */
void UArticyCloneableObject::AddClone(UArticyObject* Clone, int32 CloneId)
{
	if (!ensure(Clone))
		return;

	if (CloneId == -1)
	{
		//find the first free clone id
		for (int i = 0; i != -1; ++i)
		{
			if (!Clones.Find(i))
			{
				CloneId = i;
				break;
			}
		}
	}

	Clones.Add(CloneId, FArticyShadowableObject{ Clone, CloneId });
}

//---------------------------------------------------------------------------//

UArticyDatabase::UArticyDatabase()
{
	// Dynamically find the class that inherits from UArticyExpressoScripts
	TArray<UClass*> ExpressoScriptClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UArticyExpressoScripts::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
		{
			ExpressoScriptClasses.Add(*It);
		}
	}

	if (ExpressoScriptClasses.Num() > 0)
	{
		// There should be only one, so we use the first one found
		SetExpressoScriptsClass(ExpressoScriptClasses[0]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No class derived from UArticyExpressoScripts found."));
	}
}

/**
 * Initializes the Articy database, loading default packages.
 */
void UArticyDatabase::Init()
{
	LoadDefaultPackages();
}

/**
 * Explicit initialize.
 * @param WorldContext any UObject in the target world
 */
void UArticyDatabase::Initialize(const UObject* WorldContext)
{
	// this will clone & Init internally
	UArticyDatabase* DB = UArticyDatabase::Get(WorldContext);
	if (DB && !DB->bIsInitialized)
	{
		DB->bIsInitialized = true;
		UE_LOG(LogArticyRuntime, Verbose, TEXT("ArticyDatabase initialized for world %s"), *WorldContext->GetName());
	}
}

/**
 * Explicit deinitialize.
 * @param WorldContext any UObject in the target world
 */
void UArticyDatabase::Deinitialize(const UObject* WorldContext)
{
	UArticyDatabase* DB = UArticyDatabase::Get(WorldContext);
	if (!DB || !DB->bIsInitialized)
		return;

	DB->UnloadDatabase();
	DB->bIsInitialized = false;
	UE_LOG(LogArticyRuntime, Verbose, TEXT("ArticyDatabase deinitialized"));
}

/**
 * Retrieves the Articy database instance for the given world context.
 * @param WorldContext The context from which to retrieve the database.
 * @return A pointer to the Articy database instance.
 */
UArticyDatabase* UArticyDatabase::Get(const UObject* WorldContext)
{
#if ENGINE_MAJOR_VERSION >= 5
	bool bKeepBetweenWorlds = UArticyPluginSettings::Get()->bKeepDatabaseBetweenWorlds || WorldContext->GetWorld()->IsPartitionedWorld();
#else
	bool bKeepBetweenWorlds = UArticyPluginSettings::Get()->bKeepDatabaseBetweenWorlds;
#endif

	if (bKeepBetweenWorlds && PersistentClone.IsValid())
		return PersistentClone.Get();

	//remove all clones who's world died (world == nullptr)
	Clones.Remove(nullptr);

	auto world = GEngine->GetWorldFromContextObjectChecked(WorldContext);
	if (!ensureMsgf(world, TEXT("Could not get world from WorldContext %s"), WorldContext ? *WorldContext->GetName() : TEXT("NULL")))
		return nullptr;

	//find either the persistent clone or the clone that belongs to the world of the passed in context object
	auto& clone = bKeepBetweenWorlds ? PersistentClone : Clones.FindOrAdd(world);

	if (!clone.IsValid())
	{
		//clone not valid, create a new one
		UE_LOG(LogArticyRuntime, Log, TEXT("Cloning ArticyDatabase."))

			//get the original asset to clone from
			auto asset = GetOriginal();
		if (!asset)
			return nullptr;

		//duplicate the original asset
		if (bKeepBetweenWorlds)
		{
			clone = DuplicateObject((UArticyDatabase*)asset, world->GetGameInstance());
#if !WITH_EDITOR
			clone->AddToRoot();
#endif
		}
		else
		{
			clone = DuplicateObject((UArticyDatabase*)asset, world);
		}

		//make the clone load its default packages
		if (clone.IsValid())
			clone->Init();
	}

	return clone.Get();
}

/**
 * Unloads the database, clearing changes and freeing resources.
 */
void UArticyDatabase::UnloadDatabase()
{
	bool bKeepBetweenWorlds = UArticyPluginSettings::Get()->bKeepDatabaseBetweenWorlds;

	TWeakObjectPtr<UArticyDatabase>* dbPtr = nullptr;
	if (bKeepBetweenWorlds)
		dbPtr = &PersistentClone;
	else
		dbPtr = &Clones.FindOrAdd(GetWorld());

	if (dbPtr && dbPtr->IsValid())
	{
		(*dbPtr)->RemoveFromRoot();
		(*dbPtr)->ConditionalBeginDestroy();
		*dbPtr = NULL;
	}
}

/**
 * Sets the default user methods provider for script execution.
 * @param MethodProvider The method provider object.
 */
void UArticyDatabase::SetDefaultUserMethodsProvider(UObject* MethodProvider)
{
	GetExpressoInstance()->SetDefaultUserMethodsProvider(MethodProvider);
}

/**
 * Retrieves the current global variables instance.
 * @return A pointer to the current UArticyGlobalVariables instance.
 */
UArticyGlobalVariables* UArticyDatabase::GetGVs() const
{
	// If we have an active variable set, grab that set
	UArticyGlobalVariables* ActiveGlobals = GetExpressoInstance()->GetGV();
	if (ActiveGlobals)
		return ActiveGlobals;

	// Otherwise, return the default GVs
	return UArticyGlobalVariables::GetDefault(this);
}

/**
 * Retrieves the runtime global variables associated with an alternative global variables asset.
 * @param Asset The alternative global variables asset.
 * @return A pointer to the runtime UArticyGlobalVariables instance.
 */
UArticyGlobalVariables* UArticyDatabase::GetRuntimeGVs(UArticyAlternativeGlobalVariables* Asset) const
{
	// Find or create a clone of the global variables set associated with this "alternative" set
	return UArticyGlobalVariables::GetRuntimeClone(this, Asset);
}

/**
 * Retrieves the names of all imported packages.
 * @return An array of strings representing the imported package names.
 */
TArray<FString> UArticyDatabase::GetImportedPackageNames() const
{
	TArray<FString> outNames;
	ImportedPackages.GenerateKeyArray(outNames);
	return outNames;
}

/**
 * Determines if a package is a default package.
 * @param PackageName The name of the package to check.
 * @return True if the package is a default package, false otherwise.
 */
bool UArticyDatabase::IsPackageDefaultPackage(FString PackageName)
{
	if (ImportedPackages.Contains(PackageName))
	{
		const UArticyPackage* Package = ImportedPackages[PackageName];
		// in case the key exists but has no valid entry
		if (!Package)
		{
			return false;
		}

		return Package->bIsDefaultPackage;
	}

	return false;
}

/**
 * Loads all imported Articy objects, ensuring the packages are fully loaded.
 */
void UArticyDatabase::LoadAllObjects()
{
	GetOriginal(true);
}

/**
 * Sets the loaded packages from a provided list of packages.
 * @param Packages An array of packages to set as loaded.
 */
void UArticyDatabase::SetLoadedPackages(const TArray<UArticyPackage*> Packages)
{
	ImportedPackages.Reset();
	UnloadAllPackages();

	for (auto pkg : Packages)
	{
		ImportedPackages.Add(pkg->Name, pkg);
	}
}

//---------------------------------------------------------------------------//

/**
 * Loads all packages that are marked as default packages.
 */
void UArticyDatabase::LoadDefaultPackages()
{
	LoadAllPackages(true);
}

/**
 * Loads all imported packages, optionally only loading default packages.
 * @param bDefaultOnly If true, only default packages are loaded.
 */
void UArticyDatabase::LoadAllPackages(bool bDefaultOnly)
{
	for (const TPair<FString, UArticyPackage*>& pack : ImportedPackages)
	{
		if (!bDefaultOnly || (pack.Value && pack.Value->bIsDefaultPackage)
#if WITH_EDITOR
			//TODO add "or is edit mode"
#endif
			)
			LoadPackage(pack.Key);
	}
}

/**
 * Loads a specific package by name.
 * @param PackageName The name of the package to load.
 */
void UArticyDatabase::LoadPackage(FString PackageName)
{
	if (LoadedPackages.Contains(PackageName))
	{
		UE_LOG(LogArticyRuntime, Log, TEXT("Package %s already loaded."), *PackageName);
		return;
	}

	if (!ImportedPackages.Contains(PackageName) || ImportedPackages[PackageName] == nullptr)
	{
		UE_LOG(LogArticyRuntime, Error, TEXT("Failed to find Package %s in imported packages!"), *PackageName);
		return;
	}

	UArticyPackage* Package = ImportedPackages[PackageName];

	/*auto fileName = PackageName.Replace(TEXT(" "), TEXT("_"));
	auto pkgFile = Cast<UPackage>(::LoadPackage(nullptr, *fileName, 0));
	if(!pkgFile)
	{
		UE_LOG(LogArticyRuntime, Error, TEXT("Failed to load package file %s from filesystem!"), *fileName);
		return;
	}

	//load the package, to make sure all the contained objects are available
	pkgFile->FullyLoad();*/

	for (auto ArticyObject : Package->GetAssets())// MM_CHANGE
	{
		auto id = FArticyId(ArticyObject->GetId());

		if (!ensureMsgf(!LoadedObjectsById.Contains(id), TEXT("Object with id [%d,%d] already in list!"), id.High, id.Low))
			continue;

		auto CloneContainer = NewObject<UArticyCloneableObject>(this);
		UArticyObject* InitialClone = DuplicateObject<UArticyObject>(ArticyObject, this);
		CloneContainer->Init(InitialClone);

		LoadedObjectsById.Add(id, CloneContainer);

		if (!ArticyObject->GetTechnicalName().ToString().IsEmpty())
		{
			LoadedObjectsByName.FindOrAdd(ArticyObject->GetTechnicalName()).Objects.Add(CloneContainer);
		}
	}

	LoadedPackages.Add(PackageName);
	UE_LOG(LogArticyRuntime, Log, TEXT("Package %s loaded successfully."), *PackageName);
}

/**
 * Unloads a specific package by name.
 * @param PackageName The name of the package to unload.
 * @param bQuickUnload If true, perform a quick unload.
 * @return True if the package was successfully unloaded, false otherwise.
 */
bool UArticyDatabase::UnloadPackage(const FString PackageName, const bool bQuickUnload)
{
	if (!LoadedPackages.Contains(PackageName))
	{
		UE_LOG(LogArticyRuntime, Log, TEXT("Package %s can't be unloaded due to not being loaded in the first place."), *PackageName);
		return false;
	}

	if (!ImportedPackages.Contains(PackageName) || ImportedPackages[PackageName] == nullptr)
	{
		UE_LOG(LogArticyRuntime, Error, TEXT("Failed to find Package %s in imported packages!"), *PackageName);
		return false;
	}

	UArticyPackage* Package = ImportedPackages[PackageName];

	for (auto ArticyObject : Package->GetAssets())
	{
		FArticyId ArticyId = ArticyObject->GetId();
		FName TechnicalName = ArticyObject->GetTechnicalName();

		bool bShouldUnload = false;
		if (bQuickUnload)
		{
			bShouldUnload = true;
		}
		else
		{
			/*
			 *	An exported object can exist multiple times in different packages
			 *  In the database, there can only be one object with the same Id, so if we are unloading slowly, check if the object is contained in another package first
			 *  Only unload if it is not contained in any other package
			*/
			bool bIsAssetContained = false;
			for (const FString& LoadedPackageName : LoadedPackages)
			{
				if (!LoadedPackageName.Equals(Package->Name))
				{
					bIsAssetContained = bIsAssetContained || ImportedPackages[LoadedPackageName]->GetAssetsDict().Contains(TechnicalName);
				}
			}

			if (!bIsAssetContained)
			{
				bShouldUnload = true;
			}
		}

		if (bShouldUnload)
		{
			LoadedObjectsById.FindAndRemoveChecked(ArticyId);
			LoadedObjectsByName.FindAndRemoveChecked(TechnicalName);
		}
	}

	LoadedPackages.Remove(Package->Name);
	UE_LOG(LogArticyRuntime, Log, TEXT("Package %s unloaded successfully."), *PackageName);

	return true;
}

/**
 * Unloads all currently loaded packages, clearing object maps.
 */
void UArticyDatabase::UnloadAllPackages()
{
	LoadedPackages.Reset();
	LoadedObjectsById.Reset();
	LoadedObjectsByName.Reset();
}

/**
 * Sets the Expresso scripts class used for script execution.
 * @param NewClass The new Expresso scripts class to use.
 */
void UArticyDatabase::SetExpressoScriptsClass(TSubclassOf<UArticyExpressoScripts> NewClass)
{
	ExpressoScriptsClass = NewClass;
}

/**
 * Retrieves the original database asset, optionally loading all packages.
 * @param bLoadAllPackages If true, loads all packages.
 * @return A pointer to the original UArticyDatabase asset.
 */
const UArticyDatabase* UArticyDatabase::GetOriginal(bool bLoadAllPackages)
{
	static TWeakObjectPtr<UArticyDatabase> Asset = nullptr;

	if (!Asset.IsValid())
	{
		//create a clone of the database
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetFName(), AssetData, true);
#endif

		if (AssetData.Num() != 0)
		{
			if (AssetData.Num() > 1)
			{
				UE_LOG(LogArticyRuntime, Warning, TEXT("More than one ArticyDraftDatabase was found, this is not supported! The first one will be selected."));
			}

			Asset = Cast<UArticyDatabase>(AssetData[0].GetAsset());

			if (bLoadAllPackages && Asset.IsValid())
				Asset.Get()->LoadAllPackages();
		}
		else
		{
			UE_LOG(LogArticyRuntime, Warning, TEXT("No ArticyDraftDatabase was found."));
		}
	}

	return Asset.Get();
}

/**
 * Retrieves a mutable original instance of the Articy database.
 * @return A weak pointer to the mutable UArticyDatabase instance.
 */
TWeakObjectPtr<UArticyDatabase> UArticyDatabase::GetMutableOriginal()
{
	static TWeakObjectPtr<UArticyDatabase> Asset = nullptr;

	if (!Asset.IsValid())
	{
		//create a clone of the database
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetClassPathName(), AssetData, true);
#else
		AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetFName(), AssetData, true);
#endif
		if (AssetData.Num() != 0)
		{
			if (AssetData.Num() > 1)
			{
				UE_LOG(LogArticyRuntime, Warning, TEXT("More than one ArticyDraftDatabase was found, this is not supported! The first one will be selected."));
			}

			Asset = Cast<UArticyDatabase>(AssetData[0].GetAsset());
		}
		else
		{
			UE_LOG(LogArticyRuntime, Warning, TEXT("No ArticyDraftDatabase was found."));
		}
	}

	return Asset;
}

/**
 * Changes the default package setting for a specific package.
 * @param PackageName The name of the package.
 * @param bIsDefaultPackage True to mark as default, false otherwise.
 */
void UArticyDatabase::ChangePackageDefault(FName PackageName, bool bIsDefaultPackage)
{
	if (ImportedPackages.Contains(PackageName.ToString()))
	{
		UArticyPackage* Package = ImportedPackages[PackageName.ToString()];

		// additional check; maybe the map entry exists but has no value due to user error or bugs
		if (!Package)
		{
			UE_LOG(LogArticyRuntime, Error, TEXT("Could not change package default setting of package %s: package not found"), *PackageName.ToString());
			return;
		}

		// if the setting isn't going to change, return early
		if (Package->bIsDefaultPackage == bIsDefaultPackage)
		{
			return;
		}

		Package->bIsDefaultPackage = bIsDefaultPackage;
		Package->MarkPackageDirty();
	}
}

//---------------------------------------------------------------------------//

/**
 * Retrieves an Articy object by its ID and clone ID.
 * @param Id The ID of the object to retrieve.
 * @param CloneId The clone ID of the object.
 * @param CastTo The class to cast the object to.
 * @return A pointer to the Articy object, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetObject(FArticyId Id, int32 CloneId, TSubclassOf<class UArticyObject> CastTo) const
{
	return GetObjectInternal(Id, CloneId);
}

/**
 * Retrieves an unshadowed version of an Articy object.
 * @param Id The ID of the object to retrieve.
 * @param CloneId The clone ID of the object.
 * @return A pointer to the unshadowed Articy object, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetObjectUnshadowed(FArticyId Id, int32 CloneId) const
{
	return GetObjectInternal(Id, CloneId, true);
}

/**
 * Retrieves an Articy object by its ID and clone ID, with optional unshadowing.
 * @param Id The ID of the object to retrieve.
 * @param CloneId The clone ID of the object.
 * @param bForceUnshadowed If true, retrieves the unshadowed version.
 * @return A pointer to the Articy object, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetObjectInternal(FArticyId Id, int32 CloneId, bool bForceUnshadowed) const
{
	UArticyCloneableObject* const* info = LoadedObjectsById.Find(Id);
	return info && (*info) ? (*info)->Get(this, CloneId, bForceUnshadowed) : nullptr;
}

/**
 * Retrieves an Articy object by its technical name and clone ID.
 * @param TechnicalName The technical name of the object to retrieve.
 * @param CloneId The clone ID of the object.
 * @param CastTo The class to cast the object to.
 * @return A pointer to the Articy object, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetObjectByName(FName TechnicalName, int32 CloneId, TSubclassOf<class UArticyObject> CastTo) const
{
	auto arr = LoadedObjectsByName.Find(TechnicalName);
	if (!arr || arr->Objects.Num() <= 0)
		return nullptr;

	auto info = arr->Objects[0];
	return info ? Cast<UArticyObject>(info->Get(this, CloneId)) : nullptr;
}

/**
 * Retrieves an Articy object from a string representation.
 * @param StringID_CloneID The string representation of the object's ID and clone ID.
 * @param CastTo The class to cast the object to.
 * @return A pointer to the Articy object, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetObjectFromStringRepresentation(FString StringID_CloneID, TSubclassOf<class UArticyObject> CastTo) const
{
	FString StringId, CloneId;
	StringID_CloneID.Split(TEXT("_"), &StringId, &CloneId, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	uint64 id = FCString::Strtoui64(*StringId, NULL, 10);

	return GetObjectInternal(id, FCString::Atoi(*CloneId));
}

/**
 * Retrieves all Articy objects with a given technical name and clone ID.
 * @param TechnicalName The technical name of the objects to retrieve.
 * @param CloneId The clone ID of the objects.
 * @param CastTo The class to cast the objects to.
 * @return An array of pointers to the Articy objects, or empty if none found.
 */
TArray<UArticyObject*> UArticyDatabase::GetObjects(FName TechnicalName, int32 CloneId, TSubclassOf<class UArticyObject> CastTo) const
{
	return GetObjects<UArticyObject>(TechnicalName, CloneId);
}

/**
 * Retrieves all Articy objects of a specified class with a given clone ID.
 * @param Type The class type of the objects to retrieve.
 * @param CloneId The clone ID of the objects.
 * @return An array of pointers to the Articy objects, or empty if none found.
 */
TArray<UArticyObject*> UArticyDatabase::GetObjectsOfClass(TSubclassOf<class UArticyObject> Type, int32 CloneId) const
{
	TArray<UArticyObject*> arr;
	TArray<UArticyCloneableObject*> Objects;
	LoadedObjectsById.GenerateValueArray(Objects);

	for (auto ClonableObject : Objects)
	{
		auto obj = ClonableObject->Get(this, CloneId, /*bForceUnshadowed = */ true);
		if (obj && (obj->GetCloneId() == CloneId) && obj->IsA(Type))
			arr.Add(Cast<UArticyObject>(obj));
	}

	return arr;
}

/**
 * Retrieves all Articy objects currently loaded in the database.
 * @return An array of pointers to all Articy objects.
 */
TArray<UArticyObject*> UArticyDatabase::GetAllObjects() const
{
	TArray<UArticyObject*> arr;
	TArray<UArticyCloneableObject*> Objects;
	LoadedObjectsById.GenerateValueArray(Objects);
	for (auto ClonableObject : Objects)
	{
		auto obj = ClonableObject->Get(this, 0, /*bForceUnshadowed = */ true);
		arr.Add(obj);
	}
	return arr;
}

//---------------------------------------------------------------------------//

/**
 * Clones an Articy object by its ID, creating a new instance with the specified clone ID.
 * @param Id The ID of the object to clone.
 * @param NewCloneId The clone ID for the new instance.
 * @param CastTo The class to cast the clone to.
 * @return A pointer to the newly created clone, or nullptr if creation failed.
 */
UArticyObject* UArticyDatabase::CloneFrom(FArticyId Id, int32 NewCloneId, TSubclassOf<class UArticyObject> CastTo)
{
	auto info = LoadedObjectsById.Find(Id);
	return info ? (*info)->Clone(this, NewCloneId, true) : nullptr;
}

/**
 * Clones an Articy object by its technical name, creating a new instance with the specified clone ID.
 * @param TechnicalName The technical name of the object to clone.
 * @param NewCloneId The clone ID for the new instance.
 * @param CastTo The class to cast the clone to.
 * @return A pointer to the newly created clone, or nullptr if creation failed.
 */
UArticyObject* UArticyDatabase::CloneFromByName(FName TechnicalName, int32 NewCloneId, TSubclassOf<class UArticyObject> CastTo)
{
	auto arr = LoadedObjectsByName.Find(TechnicalName);
	if (!arr || arr->Objects.Num() <= 0)
		return nullptr;

	auto info = arr->Objects[0];
	return info ? Cast<UArticyObject>(info->Clone(this, NewCloneId, true)) : nullptr;
}

//---------------------------------------------------------------------------//

/**
 * Retrieves or clones an Articy object by its ID and clone ID.
 * @param Id The ID of the object to retrieve or clone.
 * @param NewCloneId The clone ID for the new instance.
 * @return A pointer to the retrieved or newly created clone, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetOrClone(FArticyId Id, int32 NewCloneId)
{
	auto info = LoadedObjectsById.Find(Id);
	return info ? (*info)->Clone(this, NewCloneId, false) : nullptr;
}

/**
 * Retrieves or clones an Articy object by its technical name and clone ID.
 * @param TechnicalName The technical name of the object to retrieve or clone.
 * @param NewCloneId The clone ID for the new instance.
 * @return A pointer to the retrieved or newly created clone, or nullptr if not found.
 */
UArticyObject* UArticyDatabase::GetOrCloneByName(const FName& TechnicalName, int32 NewCloneId)
{
	auto arr = LoadedObjectsByName.Find(TechnicalName);
	if (!arr || arr->Objects.Num() <= 0)
		return nullptr;

	auto info = arr->Objects[0];
	return info ? Cast<UArticyObject>(info->Clone(this, NewCloneId, false)) : nullptr;
}

/**
 * Retrieves the Expresso scripts instance for script execution.
 * @return A pointer to the UArticyExpressoScripts instance.
 */
UArticyExpressoScripts* UArticyDatabase::GetExpressoInstance() const
{
	if (!CachedExpressoScripts)
	{
		if (ensure(ExpressoScriptsClass))
		{
			CachedExpressoScripts = NewObject<UArticyExpressoScripts>(const_cast<UArticyDatabase*>(this), ExpressoScriptsClass);
			CachedExpressoScripts->Init(const_cast<UArticyDatabase*>(this));
		}
	}

	return CachedExpressoScripts;
}

/**
 * Resolves IDs from the provided Articy asset file name.
 * @param articyAssetFileName The file name of the Articy asset.
 * @return A structure containing the resolved asset ID.
 */
UArticyDatabase::FAssetId UArticyDatabase::ResolveIDs(const FString& articyAssetFileName)
{
	FString fileName = FPaths::GetBaseFilename(articyAssetFileName);
	FAssetId assetId;
	FString technicalName;
	fileName.Split(TEXT("_"), &technicalName, &(assetId.s_uniqueID), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	assetId.technicalName = FName(*technicalName);
	auto uniqueIDStart = *(assetId.s_uniqueID) + 2;
	auto uniqueIDEnd = &(assetId.s_uniqueID[assetId.s_uniqueID.Len() - 1]);
	assetId.uniqueID = FCString::Strtoui64(uniqueIDStart, &uniqueIDEnd, 16);

	return assetId;
}

/** Static map of clones associated with each world. */
TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UArticyDatabase>> UArticyDatabase::Clones;
/** Static persistent clone instance. */
TWeakObjectPtr<UArticyDatabase> UArticyDatabase::PersistentClone;
