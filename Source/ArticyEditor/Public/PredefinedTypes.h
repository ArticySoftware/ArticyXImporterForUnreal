//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObject.h"

//#include "PredefinedTypes.generated.h"

class UArticyImportData;

/**
 * Use this to define a predefined type which uses the same string for declaring a class and exposing a variable
 * The type must provide a constructor which takes a TSharedPtr<FJsonValue>, used as deserializer.
 */
#define PREDEFINE_TYPE(Type) new ArticyPredefinedTypeInfo<Type>(#Type, #Type, TEXT(""), [](PROP_SETTER_PARAMS) { return Type{ Json }; })
 //same as PREDEFINED_TYPE, but with a custom DefaultValue and Deserializer.
#define PREDEFINE_TYPE_EXT(Type, DefaultValue, Deserializer) new ArticyPredefinedTypeInfo<Type>(#Type, #Type, DefaultValue, Deserializer)
//same as PREDEFINED_TYPE_EX, but creates a ArticyObjectTypeInfo, which has a default deserializer
#define PREDEFINE_ARTICYOBJECT_TYPE(Type) new ArticyObjectTypeInfo<Type, Type*>(#Type, #Type"*")

#define PROP_SETTER_PARAMS UArticyBaseObject* Model, const FString& Path, const TSharedPtr<FJsonValue> Json, const FString& PackageName
#define ARRAY_SETTER_PARAMS UArticyBaseObject* Model, const FString& Path, const TArray<TSharedPtr<FJsonValue>> JsonArray, const FString& PackageName
#define PROP_SETTER_ARGS Model, Path, Json, PackageName

// Converts Unity rich text markup to Unreal rich text markup.
// Amounts to just replacing all closing tags with </> as Unreal
// does not include the tag name in the closing tag.
//
// Ex. "My text has <b>bold</b> words." to "My text has <b>bold</> words."
FString ConvertUnityMarkupToUnreal(const FString& Input);
FString DecodeHtmlEntities(const FString& Input);

/**
 * Base class for predefined type information.
 */
struct FArticyPredefinedTypeBase
{
	FArticyPredefinedTypeBase(const FString& TypeName, const FString& PropTypeName, const FString& DefaultValue) : CppType(TypeName), CppPropertyType(PropTypeName), CppDefaultValue(DefaultValue) { }
	virtual ~FArticyPredefinedTypeBase() = default;

	FString CppType = "";
	FString CppPropertyType = "";
	FString CppDefaultValue = "";

	virtual void SetProp(FName Property, PROP_SETTER_PARAMS) { ensureMsgf(false, TEXT("SetProp not implemented in derived class!")); }
	virtual void SetArray(FName ArrayProperty, ARRAY_SETTER_PARAMS) { ensureMsgf(false, TEXT("SetProp not implemented in derived class!")); }
};

/**
 * This struct contains information about the type (as C++ type and as string) used
 * to declare a class/struct/enum as well as the type to expose an instance of that
 * type.
 * It also defines how to initialize a property on a UArticyObject from JSON,
 * using UArticyObject->setProp.
 */
template<typename Type, typename PropType = Type>
struct ArticyPredefinedTypeInfo : FArticyPredefinedTypeBase
{
	ArticyPredefinedTypeInfo(const FString& TypeName, const FString& PropTypeName, const FString& DefaultValue, PropType(*Deserializer)(PROP_SETTER_PARAMS))
		: FArticyPredefinedTypeBase(TypeName, PropTypeName, DefaultValue), Deserializer(Deserializer)
	{ }

	virtual ~ArticyPredefinedTypeInfo() override = default;

	typedef PropType(*TDeserializer)(PROP_SETTER_PARAMS);
	/** The deserializer used to deserialize JSON to PropType. */
	TDeserializer Deserializer = nullptr;

	/** This is used to set a property of type PropType. */
	virtual void SetProp(const FName Property, PROP_SETTER_PARAMS) override
	{
		if (ensure(Deserializer))
			Model->SetProp<PropType>(Property, Deserializer(PROP_SETTER_ARGS));
	}

	/** This is used to set an array property with ItemType = PropType. */
	virtual void SetArray(FName ArrayProperty, ARRAY_SETTER_PARAMS) override
	{
		if (!ensure(Deserializer))
			return;

		static TArray<PropType> PropArray;

		PropArray.Reset(JsonArray.Num());
		for (auto j : JsonArray)
			PropArray.Add(Deserializer(Model, Path, j, PackageName));

		Model->SetProp(ArrayProperty, PropArray);
	}
};

/**
 * This ArticyPredefinedTypeInfo can be used for all UArticyBaseObjects.
 * It defines a default Deserializer.
 */
template<typename Type, typename PropType = Type>
struct ArticyObjectTypeInfo final : ArticyPredefinedTypeInfo<Type, PropType>
{
	ArticyObjectTypeInfo(FString TypeName, FString PropTypeName) : ArticyPredefinedTypeInfo<Type, PropType>(TypeName, PropTypeName, TEXT("nullptr"), [](PROP_SETTER_PARAMS)
		{
			auto Val = NewObject<Type>(Model);
			Val->InitFromJson(Json);

			UObject* Raw = Val;
			if (const auto Prim = Cast<UArticyPrimitive>(Raw))
				Model->AddSubobject(Prim);

			return Val;
		}) {
		static_assert(std::is_base_of_v<UArticyBaseObject, Type>, "ArticyObjectTypeInfo might only be used for UArticyBaseObject!");
	}

	virtual ~ArticyObjectTypeInfo() override = default;
};

/**
 * Just a container to hold information about the predefined Articy types.
 */
struct FArticyPredefTypes
{
	FArticyPredefTypes();

	static TMap<FName, FArticyPredefinedTypeBase*>& Get() { return StaticInstance.Types; }
	static ArticyPredefinedTypeInfo<uint8>* GetEnum() { return StaticInstance.Enum.Get(); }
	static bool IsPredefinedType(const FName& OriginalType);

private:

	/** All predefined types */
	TMap<FName, FArticyPredefinedTypeBase*> Types;
	/** Generic type for enums, only used for setting enum properties (as uint8) */
	TSharedPtr<ArticyPredefinedTypeInfo<uint8>> Enum;

	static FArticyPredefTypes StaticInstance;
};
