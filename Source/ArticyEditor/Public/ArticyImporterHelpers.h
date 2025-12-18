//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0 
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "UObject/Package.h"
#include "UObject/ConstructorHelpers.h"
#include "Interfaces/ArticyObjectWithPosition.h"
#include "Interfaces/ArticyNode.h"

/**
 * @namespace ArticyImporterHelpers
 * @brief Provides utility functions for importing and managing Articy assets in Unreal Engine.
 *
 * This namespace contains helper functions and utilities to facilitate the creation,
 * retrieval, and management of Articy assets within the Unreal Engine editor environment.
 */
namespace ArticyImporterHelpers
{
	/**
	 * @brief Finds or creates a package with the specified name.
	 *
	 * This function attempts to find an existing package with the given name. If it doesn't exist,
	 * it creates a new package. The package is then fully loaded.
	 *
	 * @param Name The name of the package to find or create.
	 * @return A pointer to the found or created package.
	 */
	inline UPackage* FindOrCreatePackage(const FString& Name)
	{
		const FString PackageName = ArticyHelpers::GetArticyGeneratedFolder() / Name;

		// @TODO Engine Versioning
#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
		UPackage* AssetPackage = CreatePackage(*PackageName);
#else
		UPackage* AssetPackage = CreatePackage(nullptr, *PackageName);
#endif

		AssetPackage->FullyLoad();

		return AssetPackage;
	}

	/**
	 * @brief Retrieves a class by name, optionally within a specified module.
	 *
	 * This function looks up a class by name, searching within the specified module if provided.
	 * If no module is specified, it searches within the "ArticyRuntime" and project-generated classes.
	 *
	 * @param ClassName The name of the class to retrieve.
	 * @param ModuleName The optional module name to search within.
	 * @param BaseClass The base class to use for the search.
	 * @return A pointer to the found class, or nullptr if not found.
	 */
	inline UClass* RetrieveClass(const FString& ClassName, const FString& ModuleName = "", UClass* BaseClass = UArticyObject::StaticClass())
	{
		if (!ModuleName.IsEmpty())
		{
			auto FullClassName = FString::Printf(TEXT("/Script/%s.%s"), *ModuleName, *ClassName);
			return ConstructorHelpersInternal::FindOrLoadClass(FullClassName, BaseClass);
		}

		FString FullClassName = FString::Printf(TEXT("/Script/%s.%s"), TEXT("ArticyRuntime"), *ClassName);
		UClass* Result = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, BaseClass);

		if (Result != nullptr)
		{
			return Result;
		}

		FullClassName = FString::Printf(TEXT("/Script/%s.%s"), FApp::GetProjectName(), *ClassName);
		Result = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, BaseClass);

		return Result;
	}

	/**
	 * @brief Generates an asset of the specified type.
	 *
	 * This template function generates an asset of the specified type, using the provided class
	 * and module names. It can optionally create a new asset if it doesn't already exist.
	 *
	 * @tparam AssetType The type of asset to generate.
	 * @param ClassName The name of the class for the asset.
	 * @param ModuleName The module name where the class is defined.
	 * @param AssetName The optional name of the asset to generate.
	 * @param SubFolder The optional subfolder where the asset will be created.
	 * @param AdditionalFlags Optional additional object flags for the asset.
	 * @param MustCreate Whether the asset must be newly created (if true).
	 * @return A pointer to the generated asset, or nullptr on failure.
	 */
	template <typename AssetType>
	AssetType* GenerateAsset(const TCHAR* ClassName, const TCHAR* ModuleName, const FString AssetName = "", const FString SubFolder = "", const EObjectFlags AdditionalFlags = RF_NoFlags, bool MustCreate = false)
	{
		const auto ActualAssetName = AssetName.IsEmpty() ? ClassName : AssetName;
		const auto FileName = (SubFolder.IsEmpty() ? ActualAssetName : SubFolder / ActualAssetName).Replace(TEXT(" "), TEXT("_"));

		//auto FullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), ModuleName, ClassName);

		if (auto UClass = RetrieveClass(ClassName, ModuleName, AssetType::StaticClass()))
		{
			auto AssetPackage = FindOrCreatePackage(FileName);
			EObjectFlags Flags = RF_Public | RF_Standalone;

			// If this asset is flagged as MustCreate...
			if (MustCreate)
			{
				// ... Then make sure the asset doesn't already exist.
				// If it does, throw the most helpful ensure message we can and return nullptr
				AssetType* Existing = FindObject<AssetType>(AssetPackage, *ActualAssetName);
				if (!ensureAlwaysMsgf(Existing == nullptr, TEXT(
					"Warning!! Somehow you have managed to load an old Articy Asset asset (%s) when it\n"
					"_should_ have been deleted already by DeleteGeneratedAssets() in CodeGenerator.cpp.\n"
					"This is likely caused by your Source Control plugin in Unreal restoring deleted files\n"
					"unexpectedly. If you're not sure what's going on, please contact support@articy.com for\n"
					"assistance with details on how you triggered this issue.\n"
					"Loading will be cancelled to avoid crashing Unreal."
				), *ActualAssetName))
				{
					return nullptr;
				}
			}

			// primarily added so we can add RF_ArchetypeObject to the database and GV asset creation.
			// It fixes a problem in which the CDO would not get refreshed after reimporting changes via Hot Reload
			if (AdditionalFlags != RF_NoFlags)
			{
				Flags = Flags | AdditionalFlags;
			}
			AssetType* CreatedAsset = NewObject<AssetType>(AssetPackage, UClass, *ActualAssetName, Flags);;

			// if we successfully created the asset, notify the asset registry and mark it dirty
			if (CreatedAsset)
			{
				// Notify the asset registry
				FAssetRegistryModule::AssetCreated(Cast<UObject>(CreatedAsset));

				// Mark the package dirty...
				AssetPackage->MarkPackageDirty();
			}

			return CreatedAsset;
		}

		//UE_LOG(LogArticyEditor, Error, TEXT("ArticyImporter: Could not find class %s!"), ClassName);

		return nullptr;
	}

	/**
	 * @brief Generates a sub-asset of the specified type.
	 *
	 * This template function generates a sub-asset of the specified type, using the provided class
	 * and module names. The sub-asset is created within the specified outer object.
	 *
	 * @tparam AssetType The type of sub-asset to generate.
	 * @param ClassName The name of the class for the sub-asset.
	 * @param ModuleName The module name where the class is defined.
	 * @param AssetName The optional name of the sub-asset to generate.
	 * @param Outer The outer object that will own the sub-asset.
	 * @return A pointer to the generated sub-asset, or nullptr on failure.
	 */
	template <typename AssetType>
	AssetType* GenerateSubAsset(const TCHAR* ClassName, const TCHAR* ModuleName, const FString AssetName = "", UObject* Outer = nullptr)
	{
		if (!Outer)
		{
			//UE_LOG(LogArticyEditor, Error, TEXT("Could not generate sub-asset %s: Outer is null!"), *AssetName);
			return nullptr;
		}

		const auto ActualAssetName = AssetName.IsEmpty() ? ClassName : AssetName;
		const auto FileName = ActualAssetName.Replace(TEXT(" "), TEXT("_"));

		auto FullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), ModuleName, ClassName);
		if (auto UClass = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, AssetType::StaticClass()))
		{
			// only public, not standalone, since the assets are bound to their outers
			EObjectFlags Flags = RF_Public;
			AssetType* CreatedAsset = NewObject<AssetType>(Outer, UClass, FName(*ActualAssetName), Flags);

			// if we successfully created the asset, notify the asset registry and mark it dirty
			if (CreatedAsset)
			{
				// Notify the asset registry
				FAssetRegistryModule::AssetCreated(Cast<UObject>(CreatedAsset));
			}

			return CreatedAsset;
		}

		//UE_LOG(LogArticyEditor, Error, TEXT("ArticyImporter: Could not find class %s!"), ClassName);

		return nullptr;
	}

	/**
	 * @brief Checks if the engine is running in Play In Editor (PIE) mode.
	 *
	 * This function iterates through all world contexts to determine if any of them
	 * are currently in Play In Editor mode.
	 *
	 * @return True if the engine is in PIE mode, false otherwise.
	 */
	static bool IsPlayInEditor()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World()->IsPlayInEditor())
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * @struct FCompareArticyNodeXLocation
	 * @brief Comparator for sorting Articy nodes based on their X position.
	 *
	 * This helper struct provides a comparator for sorting two Articy nodes based on their X position.
	 * If two nodes have the same X position, they are further sorted by their Y position.
	 */
	struct FCompareArticyNodeXLocation
	{
		/**
		 * @brief Comparison operator for sorting nodes based on position.
		 *
		 * @param A The first Articy node ID to compare.
		 * @param B The second Articy node ID to compare.
		 * @return True if node A is positioned before node B, false otherwise.
		 */
		FORCEINLINE bool operator()(const FArticyId& A, const FArticyId& B) const
		{
			const IArticyObjectWithPosition* AObjectWithPosition = Cast<IArticyObjectWithPosition>(UArticyObject::FindAsset(A));
			const IArticyObjectWithPosition* BObjectWithPosition = Cast<IArticyObjectWithPosition>(UArticyObject::FindAsset(B));

			if (!AObjectWithPosition || !BObjectWithPosition) return true;

			const FVector2D& APos = AObjectWithPosition->GetPosition();
			const FVector2D& BPos = BObjectWithPosition->GetPosition();

			if (APos.X == BPos.X)
			{
				return APos.Y < BPos.Y;
			}

			return APos.X < BPos.X;
		}
	};

}
