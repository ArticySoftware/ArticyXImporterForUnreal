//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Engine/DataAsset.h"
#include "ArticyBaseObject.h"
#include "ShadowStateManager.h"
#include "ArticyObject.h"
#include "ArticyPackage.h"
#include "AssetRegistry/AssetData.h"
#include "ArticyDatabase.generated.h"

class UArticyExpressoScripts;
struct FArticyId;
class UArticyGlobalVariables;
class UArticyAlternativeGlobalVariables;

/**
 * Structure representing a shadow of an Articy object.
 */
USTRUCT(BlueprintType)
struct FArticyObjectShadow
{
	GENERATED_BODY()

	/** Default constructor. */
	FArticyObjectShadow() {}
	/**
	 * Constructs a shadow object with specified parameters.
	 * @param ShadowLevel The shadow level for the object.
	 * @param Object The Articy object to shadow.
	 * @param CloneId The clone ID of the object.
	 * @param Outer The outer object context for the shadow object.
	 */
	FArticyObjectShadow(uint32 ShadowLevel, UArticyObject* Object, int32 CloneId, UObject* Outer = nullptr)
		: ShadowLevel(ShadowLevel), Object(Object), Outer(Outer), CloneId(CloneId) {}

public:
	UPROPERTY()
	uint32 ShadowLevel = 0; /**< The shadow level for the object. */
	/**
	 * Gets the underlying Articy object.
	 * @return A pointer to the underlying UArticyObject.
	 */
	UArticyObject* GetObject();
	/**
	 * Gets the clone ID of the object.
	 * @return The clone ID of the object.
	 */
	int32 GetCloneId() const { return CloneId; }
private:
	UPROPERTY()
	UArticyObject* Object = nullptr; /**< The underlying Articy object. */

	TWeakObjectPtr<UObject> Outer; /**< The outer object context. */

	int32 CloneId; /**< The clone ID of the object. */
};

/**
 * Represents a shadowable Articy object, capable of managing multiple shadows.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyShadowableObject
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	FArticyShadowableObject() = default;
	/**
	 * Constructs a shadowable object with the specified object.
	 * @param Object The Articy object to shadow.
	 * @param CloneId The clone ID of the object.
	 * @param Outer The outer object context for the shadowable object.
	 */
	explicit FArticyShadowableObject(UArticyObject* Object, int32 CloneId, UObject* Outer = nullptr);

	/**
	 * Returns the requested shadow.
	 * @param ShadowManager The manager for shadow states.
	 * @param ForceUnshadowed Force retrieval of the unshadowed version.
	 * @return A pointer to the shadowed or unshadowed UArticyObject.
	 */
	UArticyObject* Get(const IShadowStateManager* ShadowManager, bool ForceUnshadowed = false) const;

private:

	/**
	 * The original [0] object and its shadows.
	 * Shadows are guaranteed to be stored in ascending order, but
	 * the array index is not the shadow level (except for 0)!
	 */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	mutable TArray<FArticyObjectShadow> ShadowCopies;
};

/**
 * Contains a reference to a UArticyObject, and to its clones if any.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyCloneableObject : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the cloneable object with the initial clone.
	 * @param InitialClone The initial clone to add.
	 */
	void Init(UArticyObject* InitialClone) { AddClone(InitialClone, 0); }

	//========================================//

	/**
	 * Get the clone of this object with a certain CloneId.
	 * Returns nullptr if the clone does not exist.
	 * @param ShadowManager The manager for shadow states.
	 * @param CloneId The clone ID to retrieve.
	 * @param bForceUnshadowed Force retrieval of the unshadowed version.
	 * @return A pointer to the clone or nullptr if not found.
	 */
	UArticyObject* Get(const IShadowStateManager* ShadowManager, int32 CloneId = 0, bool bForceUnshadowed = false) const;
	/**
	 * Clone the original object and assign it the id CloneId.
	 * If bFailIfExists is true, nullptr is returned if the clone already exists.
	 * Otherwise, the existing clone is returned.
	 * @param ShadowManager The manager for shadow states.
	 * @param CloneId The clone ID for the new clone.
	 * @param bFailIfExists Fails if the clone already exists.
	 * @return A pointer to the newly created clone or nullptr if creation failed.
	 */
	UArticyObject* Clone(const IShadowStateManager* ShadowManager, int32 CloneId, bool bFailIfExists = true);

private:

	/**
	 * The copied instances of the same object.
	 * Clones[0] is the one that is created at startup from the object assets.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	TMap<int32, FArticyShadowableObject> Clones;

	/**
	 * Adds a clone to the Clones map.
	 * @param Clone The clone to add.
	 * @param CloneId The ID for the clone.
	 */
	void AddClone(UArticyObject* Clone, int32 CloneId);
};

/**
 * Contains an array of FArticyDatabaseObjects.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyDatabaseObjectArray
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UArticyCloneableObject*> Objects; /**< The array of Articy cloneable objects. */
};

/**
 * The database is used for accessing or cloning any articy object.
 */
UCLASS(Config = Game)
class ARTICYRUNTIME_API UArticyDatabase : public UDataAsset, public IShadowStateManager
{
	GENERATED_BODY()

public:

	UArticyDatabase();

	/**
	 * Retrieves the primary asset ID for this database.
	 * @return The primary asset ID.
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(FName(TEXT("ArticyDatabase")), GetFName()); }

	/** Initializes the Articy database, loading default packages. */
	void Init();

	/**
	 * Explicit initialize.
	 * @param WorldContext any UObject in the target world
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy|Lifecycle")
	static void Initialize(const UObject* WorldContext);

	/**
	 * Explicit deinitialize.
	 * @param WorldContext any UObject in the target world
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy|Lifecycle")
	static void Deinitialize(const UObject* WorldContext);

	/**
	 * Get the static instance of the database.
	 * @param WorldContext The context from which to retrieve the database.
	 * @return A pointer to the Articy database instance.
	 */
	static UArticyDatabase* Get(const UObject* WorldContext);

	/**
	 * Get the current GVs instance.
	 * @return A pointer to the current UArticyGlobalVariables instance.
	 */
	virtual UArticyGlobalVariables* GetGVs() const;
	/**
	 * Gets the current runtime instance of a set of GVs.
	 * @param Asset The alternative global variables asset.
	 * @return A pointer to the runtime UArticyGlobalVariables instance.
	 */
	virtual UArticyGlobalVariables* GetRuntimeGVs(UArticyAlternativeGlobalVariables* Asset) const;

	/** Unloads the database, which causes that all changes get removed. */
	UFUNCTION(BlueprintCallable, Category = "Articy")
	void UnloadDatabase();

	/**
	 * Sets a default method provider, which will be always used whenever scripts get
	 * evaluated / executed without a valid method provider.
	 * @param MethodProvider The method provider object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Script Methods")
	void SetDefaultUserMethodsProvider(UObject* MethodProvider);

	/**
	 * Returns true if the database is in shadow state.
	 * Can be used in script methods to determine if the function is called during
	 * a flow player branch calculation.
	 * @return True if the database is in shadow state, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is in shadow state?"), Category = "Script Methods")
	bool IsInShadowState() const { return GetShadowLevel() > 0; }

	/**
	 * Retrieves the names of all imported packages.
	 * @return An array of strings representing the imported package names.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get imported package names"), Category = "Articy")
	TArray<FString> GetImportedPackageNames() const;

	/**
	 * Determines if a package is a default package.
	 * @param PackageName The name of the package to check.
	 * @return True if the package is a default package, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is package default package?"), Category = "Articy")
	bool IsPackageDefaultPackage(FString PackageName);

	//---------------------------------------------------------------------------//

	/**
	 * Load all objects imported from articy, so the UE4 packages are 'fully loaded'.
	 * Used internally, so ArticyRef can find the referenced object.
	 */
	static void LoadAllObjects();

	/**
	 * Sets the loaded packages from a provided list of packages.
	 * @param Packages An array of packages to set as loaded.
	 */
	virtual void SetLoadedPackages(const TArray<UArticyPackage*> Packages);

	/** Load all packages which have the IsDefaultPackage flag set to true. */
	virtual void LoadDefaultPackages();

	/** Load all imported packages. */
	virtual void LoadAllPackages(bool bDefaultOnly = false);

	/**
	 * Load a package of a given name.
	 * @param PackageName The name of the package to load.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy")
	virtual void LoadPackage(FString PackageName);

	/**
	 * Load a package of a given name.
	 * @param PackageName The name of the package to unload.
	 * @param bQuickUnload If true, perform a quick unload.
	 * @return True if the package was successfully unloaded, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy")
	virtual bool UnloadPackage(const FString PackageName, const bool bQuickUnload);

	//---------------------------------------------------------------------------//

	/**
	 * Get an object by its Id.
	 * If a CloneId other than 0 is provided, a copy of the object with this index must exist.
	 * Otherwise a null-pointer is returned.
	 * @param Id The ID of the object to retrieve.
	 * @param CloneId The clone ID of the object.
	 * @param CastTo The class to cast the object to.
	 * @return A pointer to the Articy object, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo", AdvancedDisplay = "CloneId"))
	UArticyObject* GetObject(FArticyId Id, int32 CloneId = 0, TSubclassOf<class UArticyObject> CastTo = NULL) const;
	template<typename T>
	T* GetObject(FArticyId Id, int32 CloneId = 0) const { return Cast<T>(GetObject(Id, CloneId)); }

	/**
	 * Get an unshadowed copy of an object.
	 * Internally used by the flow player to replace nodes with unshadowed ones
	 * before returning them via the flow player callbacks.
	 * @param Id The ID of the object to retrieve.
	 * @param CloneId The clone ID of the object.
	 * @return A pointer to the unshadowed Articy object, or nullptr if not found.
	 */
	UArticyObject* GetObjectUnshadowed(FArticyId Id, int32 CloneId = 0) const;

	/**
	 * Get an object by its TechnicalName.
	 * If a CloneId other than 0 is provided, a copy of the object with this index must exist,
	 * otherwise a null-pointer is returned.
	 * Note that the TechnicalName is not unique! This will take the first matching object.
	 * @param TechnicalName The technical name of the object to retrieve.
	 * @param CloneId The clone ID of the object.
	 * @param CastTo The class to cast the object to.
	 * @return A pointer to the Articy object, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo", AdvancedDisplay = "CloneId"))
	UArticyObject* GetObjectByName(FName TechnicalName, int32 CloneId = 0, TSubclassOf<class UArticyObject> CastTo = NULL) const;
	template<typename T>
	T* GetObjectByName(FName TechnicalName, int32 CloneId = 0) const { return Cast<T>(GetObjectByName(TechnicalName, CloneId)); }

	/****
	* Get an object by its Articy String Representation.
	* Use this method to get an Articy Id from a Global Variable String typed
	* that contains the result of an Expresso GetObj(---) method.
	* @param StringID_CloneID The string representation of the object's ID and clone ID.
	* @param CastTo The class to cast the object to.
	* @return A pointer to the Articy object, or nullptr if not found.
	*****/
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo", AdvancedDisplay = "CloneId"))
	UArticyObject* GetObjectFromStringRepresentation(FString StringID_CloneID, TSubclassOf<class UArticyObject> CastTo = NULL) const;
	template<typename T>
	T* GetObjectFromStringRepresentation(FString StringID_CloneID) const { return Cast<T>(GetObjectFromStringRepresentation(StringID_CloneID)); }

	/**
	 * Get all objects with a given TechnicalName.
	 * If a CloneId other than 0 is provided, copies of the objects with this index must exist,
	 * otherwise null-pointers are returned instead.
	 * Note: this allocates a new TArray, use the other variant if you already have an array
	 * to fill the objects with!
	 * @param TechnicalName The technical name of the objects to retrieve.
	 * @param CloneId The clone ID of the objects.
	 * @return An array of pointers to the Articy objects, or empty if none found.
	 */
	template<typename T>
	TArray<T*> GetObjects(FName TechnicalName, int32 CloneId = 0) const;
	/**
	 * Get all objects with a given TechnicalName.
	 * If a CloneId other than 0 is provided, copies of the objects with this index must exist,
	 * otherwise null-pointers are returned instead.
	 * @param TechnicalName The technical name of the objects to retrieve.
	 * @param CloneId The clone ID of the objects.
	 * @param CastTo The class to cast the objects to.
	 * @return An array of pointers to the Articy objects, or empty if none found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo", AdvancedDisplay = "CloneId"))
	TArray<UArticyObject*> GetObjects(FName TechnicalName, int32 CloneId = 0, TSubclassOf<class UArticyObject> CastTo = NULL) const;

	/**
	* Get all objects with a given Type.
	* If a CloneId other than 0 is provided, copies of the objects with this index must exist,
	* otherwise they will be not added to the result.
	* @param CloneId The clone ID of the objects.
	* @return An array of pointers to the Articy objects, or empty if none found.
	*/
	template<typename T>
	TArray<T*> GetObjectsOfClass(int32 CloneId = 0) const;

	/**
	* Get all objects with a given Type.
	* If a CloneId other than 0 is provided, copies of the objects with this index must exist,
	* otherwise they will be not added to the result.
	* @param Class The class type of the objects to retrieve.
	* @param CloneId The clone ID of the objects.
	* @return An array of pointers to the Articy objects, or empty if none found.
	*/
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "Class", AdvancedDisplay = "CloneId"))
	TArray<UArticyObject*> GetObjectsOfClass(TSubclassOf<class UArticyObject> Class, int32 CloneId = 0) const;

	/**
	* Get all objects.
	* @return An array of pointers to all Articy objects.
	*/
	UFUNCTION(BlueprintCallable, Category = "Articy")
	TArray<UArticyObject*> GetAllObjects() const;

	/**
	 * Get all objects with a given TechnicalName.
	 * If a CloneId other than 0 is provided, copies of the objects with this index must exist,
	 * otherwise null-pointers are returned instead.
	 * @param Array The array to fill with pointers to the Articy objects.
	 * @param TechnicalName The technical name of the objects to retrieve.
	 * @param CloneId The clone ID of the objects.
	 */
	template<typename T = UArticyObject>
	void GetObjects(TArray<T*>& Array, FName TechnicalName, int32 CloneId = 0) const;

	//---------------------------------------------------------------------------//

	/**
	 * Clone an existing object, and assign the NewCloneId to it.
	 * If the clone already exists, nullptr is returned!
	 * If NewCloneId is -1, the next free clone Id will be used.
	 * @param Id The ID of the object to clone.
	 * @param NewCloneId The clone ID for the new instance.
	 * @param CastTo The class to cast the clone to.
	 * @return A pointer to the newly created clone, or nullptr if creation failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo"))
	UArticyObject* CloneFrom(FArticyId Id, int32 NewCloneId = -1, TSubclassOf<class UArticyObject> CastTo = NULL);
	template<typename T>
	T* CloneFrom(FArticyId Id, int32 NewCloneId = -1) { return Cast<T>(CloneFrom(Id, NewCloneId)); }

	/**
	 * Clone an existing object, and assign the NewCloneId to it.
	 * If the clone already exists, nullptr is returned!
	 * If NewCloneId is -1, the next free clone Id will be used.
	 * @param TechnicalName The technical name of the object to clone.
	 * @param NewCloneId The clone ID for the new instance.
	 * @param CastTo The class to cast the clone to.
	 * @return A pointer to the newly created clone, or nullptr if creation failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo"))
	UArticyObject* CloneFromByName(FName TechnicalName, int32 NewCloneId = -1, TSubclassOf<class UArticyObject> CastTo = NULL);
	template<typename T>
	T* CloneFrom(FName TechnicalName, int32 NewCloneId = -1) { return Cast<T>(CloneFromByName(TechnicalName, NewCloneId)); }

	/**
	 * Clone an existing object, and assign the NewCloneId to it.
	 * @param Id The ID of the object to retrieve or clone.
	 * @param NewCloneId The clone ID for the new instance.
	 * @return A pointer to the retrieved or newly created clone, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo"))
	UArticyObject* GetOrClone(FArticyId Id, int32 NewCloneId);
	template<typename T>
	T* GetOrClone(FArticyId Id, int32 NewCloneId) { return Cast<T>(GetOrClone(Id, NewCloneId)); }

	/**
	 * Clone an existing object, and assign the NewCloneId to it.
	 * @param TechnicalName The technical name of the object to retrieve or clone.
	 * @param NewCloneId The clone ID for the new instance.
	 * @return A pointer to the retrieved or newly created clone, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy", meta = (DeterminesOutputType = "CastTo"))
	UArticyObject* GetOrCloneByName(const FName& TechnicalName, int32 NewCloneId);
	template<typename T>
	T* GetOrClone(const FName& TechnicalName, int32 NewCloneId) { return Cast<T>(GetOrCloneByName(TechnicalName, NewCloneId)); }

	//---------------------------------------------------------------------------//

	/**
	 * Gets a UArticyExpressoScripts instance, which can be used to execute
	 * instructions or evaluate conditions.
	 * @return A pointer to the UArticyExpressoScripts instance.
	 */
	UArticyExpressoScripts* GetExpressoInstance() const;

	/** Retrieves a mutable original instance of the Articy database.
	 * @return A weak pointer to the mutable UArticyDatabase instance.
	 */
	static TWeakObjectPtr<UArticyDatabase> GetMutableOriginal();

	/**
	 * Changes the default package setting for a specific package.
	 * @param PackageName The name of the package.
	 * @param bIsDefaultPackage True to mark as default, false otherwise.
	 */
	void ChangePackageDefault(FName PackageName, bool bIsDefaultPackage);

	UFUNCTION(BlueprintCallable, Category = "Articy")
	void SetExpressoScriptsClass(TSubclassOf<UArticyExpressoScripts> NewClass);

protected:

	/** A list of all packages that were imported from articy:draft. */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	TMap<FString, UArticyPackage*> ImportedPackages;

	UPROPERTY(VisibleAnywhere, transient, Category = "Articy")
	TArray<FString> LoadedPackages;

	UPROPERTY()
	TMap<FArticyId, UArticyCloneableObject*> LoadedObjectsById;
	UPROPERTY()
	TMap<FName, FArticyDatabaseObjectArray> LoadedObjectsByName;

	UPROPERTY(Transient)
	bool bIsInitialized = false;

	void UnloadAllPackages();

private:

	static TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UArticyDatabase>> Clones;
	static TWeakObjectPtr<UArticyDatabase> PersistentClone;

	UPROPERTY()
	mutable UArticyExpressoScripts* CachedExpressoScripts;

	/** An instance of this class will be used to execute script fragments. */
	UPROPERTY(Config, VisibleAnywhere, Category = "Articy")
	TSubclassOf<UArticyExpressoScripts> ExpressoScriptsClass;

	UArticyObject* GetObjectInternal(FArticyId Id, int32 CloneId = 0, bool bForceUnshadowed = false) const;

	/** Get the original asset (on disk) of the database.
	 * @param bLoadDefaultPackages If true, loads all packages.
	 * @return A pointer to the original UArticyDatabase asset.
	 */
	static const UArticyDatabase* GetOriginal(bool bLoadDefaultPackages = false);

	/** Extracts technical name and unique id from the articy asset file name.
	 * @param articyAssetFileName The file name of the Articy asset.
	 * @return A structure containing the resolved asset ID.
	 */
	struct FAssetId
	{
		FName technicalName;
		FString s_uniqueID;
		uint64 uniqueID;
	};

	/** Extracts technical name and unique id from the articy asset file name. */
	static FAssetId ResolveIDs(const FString& articyAssetFileName);
};

template<typename T>
TArray<T*> UArticyDatabase::GetObjects(FName TechnicalName, int32 CloneId) const
{
	static TArray<T*> Array;
	Array.Reset();

	GetObjects(Array, TechnicalName, CloneId);

	return Array;
}

template<typename T>
TArray<T*> UArticyDatabase::GetObjectsOfClass(int32 CloneId) const
{
	TArray<T*> arr;

	TArray<UArticyCloneableObject*> ArticyObjects;
	LoadedObjectsById.GenerateValueArray(ArticyObjects);

	for (auto obj : ArticyObjects)
	{
		/*if (obj->GetCloneId() == CloneId && Cast<T>(obj))
		{
			arr.Add(obj);
		}*/

		UArticyObject* Object = obj->Get(this, CloneId, false);
		if (Object)
		{
			T* TypedObject = Cast<T>(Object);
			if (TypedObject)
			{
				arr.Add(TypedObject);
			}
		}
	}
	return arr;
}

template<typename T>
void UArticyDatabase::GetObjects(TArray<T*>& Array, FName TechnicalName, int32 CloneId) const
{
	//find all objects with this name
	auto arr = LoadedObjectsByName.Find(TechnicalName);
	if (arr)
	{
		for (auto obj : arr->Objects)
		{
			auto clone = Cast<T>(obj->Clone(this, CloneId));
			if (clone)
				Array.Add(clone);
		}
	}
}
