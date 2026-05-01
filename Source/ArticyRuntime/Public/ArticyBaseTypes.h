//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyHelpers.h"
#include "ArticyBaseTypes.generated.h"

class UArticyObject;

/**
 * 64-bit ID used for referencing Articy objects, exposable to blueprints.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyId
{
	GENERATED_BODY()

public:

	FArticyId() = default;

	// ReSharper disable once CppNonExplicitConvertingConstructor
	/** Implicit conversion from uint64. */
	FArticyId(const uint64& Value) { *this = Value; }
	// ReSharper disable once CppNonExplicitConvertingConstructor
	/** Implicit conversion from FString. */
	FArticyId(const FString& Value) { *this = Value; }
	/** Json deserializer. */
	FArticyId(const TSharedPtr<FJsonValue> Json) { *this = (Json->Type == EJson::String ? ArticyHelpers::HexToUint64(Json->AsString()) : -1); }

	//========================================//

	/**
	 * Initializes the FArticyId from a string containing "Low=" and "High=".
	 * Used in custom UI elements like pins where direct access is often not possible.
	 *
	 * @param InSourceString The string to initialize the ID from.
	 * @return true if initialization is successful, otherwise false.
	 */
	bool InitFromString(const FString InSourceString);

	/** The lower 32 bit of the ID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	int32 Low = 0;

	/** The higher 32 bit of the ID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	int32 High = 0;

	//========================================//

	/** Retrieves the 64-bit representation of the ID. */
	uint64 Get() const { return static_cast<uint64>(High) << 32 | static_cast<uint64>(Low); }

	/** Implicit assignment of uint64. */
	FArticyId& operator=(const uint64& Value)
	{
		//the lower 32 bit are stored in Low, the higher 32 bit in high
		Low = static_cast<int32>(Value & 0xFFFFFFFF);
		High = static_cast<int32>(Value >> 32);

		return *this;
	}
	/** Implicit assignment of FString. */
	FArticyId& operator=(const FString& Value)
	{
		return *this = ArticyHelpers::HexToUint64(Value);
	}

	/** Implicit conversion to uint64. */
	operator uint64 const() const { return Get(); }

	/**
	 * Retrieves the referenced object (single runtime copy of the referenced asset).
	 *
	 * @param WorldContext The world context object.
	 * @return A pointer to the UArticyPrimitive object if found, otherwise nullptr.
	 */
	class UArticyPrimitive* GetObject(const UObject* WorldContext) const;

	/**
	 * Retrieves the referenced object, cast to T.
	 *
	 * @tparam T The type to cast the object to.
	 * @param WorldContext The world context object.
	 * @return A pointer to the object of type T if found and cast is successful, otherwise nullptr.
	 */
	template<typename T>
	T* GetObject(const UObject* WorldContext) const { return Cast<T>(GetObject(WorldContext)); }

	//========================================//

	/**
	 * Gets the hash of the FArticyId.
	 *
	 * @param Id The FArticyId to get the hash for.
	 * @return The hash value of the ID.
	 */
	friend uint32 GetTypeHash(const FArticyId& Id)
	{
		return Id.Low ^ Id.High;
	}

	/** Checks if the ID is null (both Low and High are zero). */
	bool IsNull() const
	{
		return High == 0 && Low == 0;
	}

	/**
	 * Converts the ID to a string format "(Low=..., High=...)".
	 * This function is primarily used for custom UI works that require IDs to be set via String.
	 *
	 * @return A string representation of the ID.
	 */
	FString ToString() const
	{
		return FString::Printf(TEXT("(Low=%d, High=%d)"), Low, High);
	}

	/**
	 * Converts the ID to an asset-friendly string format "Low_High".
	 * This function is used instead of the normal ToString function when asset-friendly names are required (no commas, etc.).
	 *
	 * @return An asset-friendly string representation of the ID.
	 */
	FString ToAssetFriendlyString() const
	{
		return FString::Printf(TEXT("%d_%d"), Low, High);
	}
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

/**
 * A size given as width and height.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticySize
{
	GENERATED_BODY()

public:

	FArticySize() = default;
	/** Constructs an FArticySize from a JSON value. */
	FArticySize(TSharedPtr<FJsonValue> Json);

	/** The width of the size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float w = 0.f;
	/** The height of the size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float h = 0.f;

	/** Converts the size to a FVector2D. */
	FVector2D AsVector() const { return FVector2D{ w, h }; }
};

/**
 * A rectangle with x/y position and w/h dimensions.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyRect
{
	GENERATED_BODY()

public:

	FArticyRect() = default;
	/** Constructs an FArticyRect from a JSON value. */
	FArticyRect(TSharedPtr<FJsonValue> Json);

	/** The x position of the rectangle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float x = 0.f;
	/** The y position of the rectangle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float y = 0.f;
	/** The width of the rectangle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float w = 0.f;
	/** The height of the rectangle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	float h = 0.f;
};

//---------------------------------------------------------------------------//

/**
 * Enumeration of view box modes for preview images.
 */
UENUM(BlueprintType)
enum class EArticyPreviewImageViewBoxModes : uint8
{
	FromAsset = 0,  /**< Use the view box mode from the asset. */
	Custom = 1,     /**< Use a custom view box mode. */
};

/**
 * Enumeration of location anchor sizes.
 */
UENUM()
enum class EArticyLocationAnchorSize : uint8
{
	Small = 0,   /**< Small anchor size. */
	Medium = 1,  /**< Medium anchor size. */
	Large = 2,   /**< Large anchor size. */
};

/**
 * A string that is not localized.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyStdString
{
	GENERATED_BODY()

public:

	FArticyStdString() = default;
	/** Constructs an FArticyStdString from a JSON value. */
	FArticyStdString(const TSharedPtr<FJsonValue>& Json);

	/** The text content of the string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
	FText Text;
};

/**
 * A string that is localized.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyMultiLanguageString : public FArticyStdString
{
	GENERATED_BODY()

};
