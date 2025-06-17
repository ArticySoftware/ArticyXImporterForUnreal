//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/Paths.h"

/**
 * Loads the asset referenced by this Articy asset.
 *
 * @return A pointer to the loaded UObject, or nullptr if the asset could not be loaded.
 */
UObject* UArticyAsset::LoadAsset() const
{
	//if(!Asset.IsValid())
	{
		/*static TArray<FAssetData> assets;

		if(assets.Num() == 0)
		{
			static TWeakObjectPtr<UObjectLibrary> lib = nullptr;
			if(!lib.IsValid())
				lib = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, GIsEditor);

			lib->LoadAssetDataFromPath(ArticyHelpers::ArticyAssetsFolder);
			lib->GetAssetDataList(assets);
		}*/

		const auto& folder = FPaths::GetPath(AssetRef);
		const auto& filename = FPaths::GetBaseFilename(AssetRef); //without extension

		//construct the asset path like UE4 wants it
		auto path = ArticyHelpers::GetArticyResourcesFolder() / folder / filename;
		//UE_LOG(LogTemp, Warning, TEXT("Asset Path %s"), *path)
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 24
		Asset = ConstructorHelpersInternal::FindOrLoadObject<UObject>(path);
#else
		Asset = ConstructorHelpersInternal::FindOrLoadObject<UObject>(path, LOAD_None);
#endif

	}

	return Asset.Get();
}

/**
 * Loads the asset as a texture.
 *
 * @return A pointer to the loaded UTexture, or nullptr if the asset could not be loaded as a texture.
 */
UTexture* UArticyAsset::LoadAsTexture() const
{
	return Cast<UTexture>(LoadAsset());
}

/**
 * Loads the asset as a 2D texture.
 *
 * @return A pointer to the loaded UTexture2D, or nullptr if the asset could not be loaded as a 2D texture.
 */
UTexture2D* UArticyAsset::LoadAsTexture2D() const
{
	return Cast<UTexture2D>(LoadAsset());
}

/**
 * Loads the asset as a sound wave.
 *
 * @return A pointer to the loaded USoundWave, or nullptr if the asset could not be loaded as a sound wave.
 */
USoundWave* UArticyAsset::LoadAsSoundWave() const
{
	return Cast<USoundWave>(LoadAsset());
}

/**
 * Loads the asset as a file media source.
 *
 * @return A pointer to the loaded UFileMediaSource, or nullptr if the asset could not be loaded as a file media source.
 */
UFileMediaSource* UArticyAsset::LoadAsFileMediaSource() const
{
	return Cast<UFileMediaSource>(LoadAsset());
}
