//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include <sstream>

#include "ArticyPluginSettings.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0 
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ArticyTextExtension.h"
#include "ArticyRuntimeModule.h"
#include "ArticyLocalizerSystem.h"

namespace ArticyHelpers
{

#define STRINGIFY(x) #x

#define JSON_SECTION_SETTINGS TEXT("Settings")
#define JSON_SECTION_PROJECT TEXT("Project")
#define JSON_SECTION_LANGUAGES TEXT("Languages")
#define JSON_SECTION_GLOBALVARS TEXT("GlobalVariables")
#define JSON_SECTION_SCRIPTMEETHODS TEXT("ScriptMethods")
#define JSON_SECTION_OBJECTDEFS TEXT("ObjectDefinitions")
#define JSON_SECTION_PACKAGES TEXT("Packages")
#define JSON_SECTION_HIERARCHY TEXT("Hierarchy")

#define JSON_SUBSECTION_TYPES TEXT("Types")
#define JSON_SUBSECTION_TEXTS TEXT("Texts")
#define JSON_SUBSECTION_OBJECTS TEXT("Objects")

/** Creates a new FJsonObject with name "x" from json->GetObjectField("x") */
#define JSON_OBJECT(json, x) TSharedPtr<FJsonObject> x = json->GetObjectField(TEXT(#x))
/** Tries to get the object with name "x" and if it's an object, executes body. */
#define JSON_TRY_OBJECT(json, x, body) static_assert(!std::is_const<decltype(x)>::value, #x " is const!"); \
	{ const TSharedPtr<FJsonObject> *obj; \
	if(json->TryGetObjectField(TEXT(#x), obj)) \
	{ body } }

/** Tries to get the bool with name "x" from json and stores it into 'x'. */
#define JSON_TRY_BOOL(json, x) static_assert(!std::is_const<decltype(x)>::value, #x " is const!"); \
	json->TryGetBoolField(TEXT(#x), x)

/** Tries to get the string with name "x" from json and stores it into 'x'. */
#define JSON_TRY_STRING(json, x) static_assert(!std::is_const<decltype(x)>::value, #x " is const!"); \
	json->TryGetStringField(TEXT(#x), x)
/** Tries to get the string with name "x" from json and stores it into 'x'. */
#define JSON_TRY_FNAME(json, x) { FString str; if(json->TryGetStringField(TEXT(#x), str)) x = *str; }

#define JSON_TRY_TEXT(json, x) { FString str; if(json->TryGetStringField(TEXT(#x), str)) x = FText::FromString(str); }

/** Tries to get the string with name "x" from json, converts it to uint64 and stores it into 'x' of type FArticyId. */
#define JSON_TRY_HEX_ID(json, x) static_assert(std::is_same<decltype(x), FArticyId>::value, #x " is not a uint64!"); \
	{ FString hex; \
	json->TryGetStringField(TEXT(#x), hex); \
	x = ArticyHelpers::HexToUint64(hex); }

/** Tries to get all the elements in an array with name "x" from json, and iterates over them. */
#define JSON_TRY_ARRAY(json, x, loopBody) \
	static_assert(!std::is_const<decltype(x)>::value, #x " is const!"); \
	{ const TArray<TSharedPtr<FJsonValue>>* items; \
	if(json->TryGetArrayField(TEXT(#x), items)) \
	for(const auto& item : *items) \
	{ loopBody } }

#define JSON_TRY_STRING_ARRAY(json, x) JSON_TRY_ARRAY(json, x, x.Add(item->AsString()); )

/** Tries to get the int with name "x" from json and stores it into 'x'. */
#define JSON_TRY_INT(json, x) json->TryGetNumberField(TEXT(#x), x)

/** Tries to get the int with name "x" from json and stores it into 'x'. */
#define JSON_TRY_FLOAT(json, x) { double d##x; json->TryGetNumberField(TEXT(#x), d##x); x = d##x; }

/** Tries to get the int with name "x" from json and stores it into 'x'. */
#define JSON_TRY_ENUM(json, x) int val; if(json->TryGetNumberField(TEXT(#x), val)) x = static_cast<decltype(x)>(val);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

	/** Returns the virtual path of the configured articy directory (e.g. "/Game/ArticyContent"). */
	inline FString GetArticyFolder()
	{
		return GetDefault<UArticyPluginSettings>()->ArticyDirectory;
	}

	/** Returns the virtual path of the ArticyContent/Resources folder. */
	inline FString GetArticyResourcesFolder()
	{
		return GetArticyFolder() / TEXT("ArticyContent") / TEXT("Resources");
	}

	/** Returns the virtual path of the ArticyContent/Generated folder where imported assets live. */
	inline FString GetArticyGeneratedFolder()
	{
		return GetArticyFolder() / TEXT("ArticyContent") / TEXT("Generated");
	}

	/** Parses a hex string (e.g. "0x1234") into a 64-bit unsigned integer. */
	inline uint64 HexToUint64(FString str) { return FCString::Strtoui64(*str, nullptr, 16); }
	/** Formats a 64-bit unsigned integer as a "0x..."-prefixed hex string. */
	inline FString Uint64ToHex(uint64 id)
	{
		std::stringstream stream;
		stream << "0x" << std::hex << id;
		return UTF8_TO_TCHAR(stream.str().c_str());
	}

	/** Formats a 64-bit articy object id as the "<id>_0" string used for the generated asset name. */
	inline FString Uint64ToObjectString(uint64 id)
	{
		std::ostringstream stream;
		stream << id << "_0";
		return UTF8_TO_TCHAR(stream.str().c_str());
	}

	/**
	 * Parses a JSON object of shape { "x": <num>, "y": <num> } into an FVector2D.
	 *
	 * @param Json The JSON value to parse.
	 * @return The parsed FVector2D, or a zero vector if Json is invalid.
	 */
	inline FVector2D ParseFVector2DFromJson(const TSharedPtr<FJsonValue> Json)
	{
		if(!Json.IsValid() || !ensure(Json->Type == EJson::Object))
			return FVector2D{};

		double X = 0, Y = 0;

		auto obj = Json->AsObject();
		obj->TryGetNumberField(TEXT("x"), X);
		obj->TryGetNumberField(TEXT("y"), Y);

		return FVector2D{ static_cast<float>(X), static_cast<float>(Y) };
	}

	/**
	 * Parses an articy-exported 3x3 2D matrix (9-element JSON array) into an Unreal 4x4 FMatrix,
	 * re-mapping the translation row into the last column.
	 *
	 * @param Json The JSON value to parse.
	 * @return The parsed FMatrix, or FMatrix::Identity if Json is invalid.
	 */
	inline FMatrix ParseFMatrixFromJson(const TSharedPtr<FJsonValue> Json)
	{
		if (!Json.IsValid() || !ensure(Json->Type == EJson::Array))
			return FMatrix::Identity;

		auto JsonArray = Json->AsArray();
		if (!ensure(JsonArray.Num() == 9))
			return FMatrix::Identity;

		TArray< float > FloatArray = TArray<float>();
		for (auto& JsonFloatValue : JsonArray)
		{
			FloatArray.Add(static_cast<float>(JsonFloatValue->AsNumber()));
		}

		// Take the 2D 3x3 Matrix from Articy:draft and conver it to a 3D 4x4 Matrix for Unreal
		return FMatrix{
			FPlane{FloatArray[0], FloatArray[1], FloatArray[2], 0.f},
			FPlane{FloatArray[3], FloatArray[4], FloatArray[5], 0.f},
			FPlane{0.f, 0.f, FloatArray[8], 0.f},
			// Translation values need to be moved over as they're always on the last column of the matrix
			FPlane{FloatArray[6], FloatArray[7], 0.f, 1.f},
		};
	}

	/**
	 * Parses a JSON object of shape { "r": <num>, "g": <num>, "b": <num>, "a": <num> } into an FLinearColor.
	 * Alpha defaults to 1.0 if absent.
	 *
	 * @param Json The JSON value to parse.
	 * @return The parsed FLinearColor, or a default-constructed FLinearColor if Json is invalid.
	 */
	inline FLinearColor ParseColorFromJson(const TSharedPtr<FJsonValue> Json)
	{
		if(!Json.IsValid() || !ensure(Json->Type == EJson::Object))
			return FLinearColor{};

		double R, G, B, A = 1.0;

		auto obj = Json->AsObject();
		obj->TryGetNumberField(TEXT("r"), R);
		obj->TryGetNumberField(TEXT("g"), G);
		obj->TryGetNumberField(TEXT("b"), B);
		obj->TryGetNumberField(TEXT("a"), A);

		return FLinearColor{ static_cast<float>(R), static_cast<float>(G), static_cast<float>(B), static_cast<float>(A) };
	}

	/**
	 * Resolves variable interpolation inside an articy text through the localizer system.
	 * If no localizer is available, returns SourceText unchanged.
	 *
	 * @param Outer The object context used for variable lookup.
	 * @param SourceText The text to resolve.
	 * @return The resolved FText.
	 */
	inline FText ResolveText(UObject* Outer, const FText* SourceText)
	{
		UArticyLocalizerSystem* ArticyLocalizerSystem = UArticyLocalizerSystem::Get();
		if (!ArticyLocalizerSystem)
		{
			return *SourceText;
		}
		return ArticyLocalizerSystem->ResolveText(Outer, SourceText);
	}

	/**
	 * Looks up Key in the articy string table and optionally resolves variable interpolation.
	 * If no localizer is available, returns BackupText (when provided) or Key.
	 *
	 * @param Outer The object context used for variable lookup.
	 * @param Key The string table key to look up.
	 * @param ResolveTextExtension Whether to resolve variable interpolation on the result.
	 * @param BackupText Optional fallback text used when the localizer is unavailable or the key is missing.
	 * @return The localized FText.
	 */
	inline FText LocalizeString(UObject* Outer, const FText& Key, bool ResolveTextExtension = true, const FText* BackupText = nullptr)
	{
		UArticyLocalizerSystem* ArticyLocalizerSystem = UArticyLocalizerSystem::Get();
		if (!ArticyLocalizerSystem)
		{
			if (BackupText)
			{
				return *BackupText;
			}
			return Key;
		}
		return ArticyLocalizerSystem->LocalizeString(Outer, Key, ResolveTextExtension, BackupText);
	}

}


