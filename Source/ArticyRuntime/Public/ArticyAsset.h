//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "FileMediaSource.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "ArticyAsset.generated.h"

/**
 * Enum to categorize different types of assets.
 */
UENUM(BlueprintType)
enum class EArticyAssetCategory : uint8
{
	None = 0, ///< No specific category

	/** All image-related asset types, including icons and SVG. */
	Image = 1,
	/** All video-related asset types. */
	Video = 2,
	/** All audio-related asset types. */
	Audio = 4,
	/** All document-related asset types, such as TXT and DOC. */
	Document = 8,
	/** Miscellaneous asset types, such as OBJ, BLEND, FBX, etc. */
	Misc = 16,

	/** Flag to describe all assets, used when declaring supported assets. */
	All = 0xFF,
};

/**
 * Base class for all imported assets.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyAsset : public UArticyObject
{
	GENERATED_BODY()

public:

	/**
	 * Loads the asset referenced by this Articy asset.
	 *
	 * @return A pointer to the loaded UObject, or nullptr if the asset could not be loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Load Asset")
	UObject* LoadAsset() const;

	/**
	 * Loads the asset as a texture.
	 *
	 * @return A pointer to the loaded UTexture, or nullptr if the asset could not be loaded as a texture.
	 */
	UFUNCTION(BlueprintCallable, Category = "Load Asset")
	UTexture* LoadAsTexture() const;

	/**
	 * Loads the asset as a 2D texture.
	 *
	 * @return A pointer to the loaded UTexture2D, or nullptr if the asset could not be loaded as a 2D texture.
	 */
	UFUNCTION(BlueprintCallable, Category = "Load Asset")
	UTexture2D* LoadAsTexture2D() const;

	/**
	 * Loads the asset as a sound wave.
	 *
	 * @return A pointer to the loaded USoundWave, or nullptr if the asset could not be loaded as a sound wave.
	 */
	UFUNCTION(BlueprintCallable, Category = "Load Asset")
	USoundWave* LoadAsSoundWave() const;

	/**
	 * Loads the asset as a file media source.
	 *
	 * @return A pointer to the loaded UFileMediaSource, or nullptr if the asset could not be loaded as a file media source.
	 */
	UFUNCTION(BlueprintCallable, Category = "Load Asset")
	UFileMediaSource* LoadAsFileMediaSource() const;

	/** The relative path of the referenced asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meta Data")
	FString AssetRef;
	/** The category of the referenced asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meta Data")
	EArticyAssetCategory Category;

private:
	/** A transient pointer to the asset, used to cache the loaded asset. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Articy")
	mutable TWeakObjectPtr<UObject> Asset = nullptr;
};
