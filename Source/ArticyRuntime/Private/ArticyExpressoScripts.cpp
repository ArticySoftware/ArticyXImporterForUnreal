//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyExpressoScripts.h"
#include "ArticyRuntimeModule.h"
#include "ArticyFlowPlayer.h"
#include <ArticyPins.h>

TMap<FName, ExpressoType::Definition> ExpressoType::Definitions;

ExpressoType::ExpressoType() {}
ExpressoType::~ExpressoType() {}

/**
 * @brief Constructs an ExpressoType from an object and a property name.
 *
 * This constructor initializes an ExpressoType instance based on the specified property of a given object.
 *
 * @param Object The object containing the property.
 * @param Property The name of the property.
 */
ExpressoType::ExpressoType(UArticyBaseObject* Object, const FString& Property)
{
	auto propName = Property;
	Object = TryFeatureReroute(Object, propName);

	if (!Object)
		return;

	auto prop = Object->GetProperty(*propName);
	if (!ensure(prop))
		return;

	FString itemType;
	FName type = *prop->GetCPPType(&itemType);

	auto factory = GetDefinition(type).Factory;
	if (ensureMsgf(factory, TEXT("Property %s has unknown type %s!"), *propName, *type.ToString()))
		*this = factory(Object, prop);
}

//---------------------------------------------------------------------------//

/**
 * @brief Constructs an ExpressoType from a boolean value.
 *
 * @param Value The boolean value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const bool& Value)
{
	BoolValue = Value;
	Type = Bool;
}

/**
 * @brief Constructs an ExpressoType from an int64 value.
 *
 * @param Value The int64 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const int64& Value)
{
	IntValue = Value;
	Type = Int;
}

/**
 * @brief Constructs an ExpressoType from a double value.
 *
 * @param Value The double value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const double& Value)
{
	FloatValue = Value;
	Type = Float;
}

/**
 * @brief Constructs an ExpressoType from a string value.
 *
 * @param Value The string value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const FString& Value)
{
	StringValue = Value;
	Type = String;
}

//========================================//

/**
 * @brief Constructs an ExpressoType from an ArticyPrimitive object.
 *
 * This constructor initializes an ExpressoType instance based on the ID and clone ID of the specified object.
 *
 * @param Object The ArticyPrimitive object.
 */
ExpressoType::ExpressoType(const UArticyPrimitive* Object)
{
	Type = String;

	if (!Object)
	{
		// Return "Null" ID to avoid crashes expecting this to be in the %11u_%d format
		StringValue = "0_0";
		return;
	}

	// Make sure to use the full 64-bit ID
	StringValue = FString::Printf(TEXT("%llu_%d"), Object->GetId().Get(), Object->GetCloneId());
}

/**
 * @brief Constructs an ExpressoType from an ArticyString value.
 *
 * @param Value The ArticyString value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const UArticyString& Value)
{
	Type = String;
	StringValue = Value.Get();
}

/**
 * @brief Constructs an ExpressoType from an ArticyInt value.
 *
 * @param Value The ArticyInt value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const UArticyInt& Value)
{
	Type = Int;
	IntValue = Value.Get();
}

/**
 * @brief Constructs an ExpressoType from an ArticyBool value.
 *
 * @param Value The ArticyBool value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const UArticyBool& Value)
{
	Type = Bool;
	BoolValue = Value.Get();
}

/**
 * @brief Constructs an ExpressoType from an ArticyId value.
 *
 * @param Value The ArticyId value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const FArticyId& Value)
{
	Type = String;

	if (!Value)
	{
		// Return "Null" ID to avoid crashes expecting this to be in the %11u_%d format
		StringValue = "0_0";
		return;
	}

	// Make sure to use the full 64-bit ID
	// > always 0 for clone ID... PB...
	StringValue = FString::Printf(TEXT("%llu_0"), Value.Get());
}

/**
 * @brief Conversion operator to boolean.
 *
 * This operator converts the ExpressoType instance to a boolean value.
 *
 * @return The boolean value of the ExpressoType.
 */
ExpressoType::operator bool() const
{
	ensure(Type == Bool);
	return GetBool();
}

/**
 * @brief Conversion operator to int64.
 *
 * This operator converts the ExpressoType instance to an int64 value.
 *
 * @return The int64 value of the ExpressoType.
 */
ExpressoType::operator int64() const
{
	ensure(Type == Float || Type == Int || Type == String);

	if (Type == String)
	{
		// If we're trying to cast a string to an int64, that likely means we're trying
		//  to assign an ArticyId via an Articy Primitive (returned by getObj or the like).
		// In that case, the string should be of the format 0xARTICYID_0xCLONEID
		// So, split the string along underscores (_)
		TArray<FString> articyIds;
		int32 numberOfParts = GetString().ParseIntoArray(articyIds, TEXT("_"), true);
		if (!ensureMsgf(numberOfParts == 2, TEXT(
			"Trying to convert a string to 64-bit integer (such as an Articy ID). "
			"Only the result of getObj or similar methods can be assigned to Slots.")))
		{
			// Fail with 0x00000000
			return 0;
		}

		// Now, convert
		return FCString::Atoi64(*articyIds[0]);
	}

	if (Type == Float)
		return GetFloat();

	return GetInt();
}

/**
 * @brief Conversion operator to double.
 *
 * This operator converts the ExpressoType instance to a double value.
 *
 * @return The double value of the ExpressoType.
 */
ExpressoType::operator double() const
{
	ensure(Type == Float || Type == Int);

	if (Type == Float)
		return GetFloat();

	return GetInt();
}

/**
 * @brief Conversion operator to FString.
 *
 * This operator converts the ExpressoType instance to an FString.
 *
 * @return The FString value of the ExpressoType.
 */
ExpressoType::operator FString() const
{
	ensure(Type == String);
	return GetString();
}

/**
 * @brief Converts the ExpressoType instance to a string representation.
 *
 * This method returns the string representation of the ExpressoType based on its type.
 *
 * @return The string representation of the ExpressoType.
 */
FString ExpressoType::ToString() const
{
	// If we're a string, then just return that string
	if (Type == String)
		return GetString();

	// For integers, use FString::FromInt
	if (Type == Int)
		return FString::FromInt(GetInt());

	// For floats, use format
	if (Type == Float)
		return FString::SanitizeFloat(GetFloat());

	ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	return FString();
}

//---------------------------------------------------------------------------//
//===========================================================================//

/**
 * @brief Negates the ExpressoType instance.
 *
 * This operator returns a new ExpressoType instance with the negated value of the current instance.
 *
 * @return The negated ExpressoType instance.
 */
ExpressoType ExpressoType::operator-() const
{
	switch (Type)
	{
	case Undefined:
		break;
	case Bool:
		return ExpressoType(!GetBool());
	case Int:
		return ExpressoType(-GetInt());
	case Float:
		return ExpressoType(-GetFloat());
	case String:
		return ExpressoType(FString(""));

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

//---------------------------------------------------------------------------//

/**
 * @brief Equality operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for equality.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the instances are equal, false otherwise.
 */
bool ExpressoType::operator==(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Undefined:
		break;

	case Bool:
		return GetBool() == Other.GetBool();
	case Int:
		switch (Other.Type)
		{
		case Int:
			return GetInt() == Other.GetInt();
		case Float:
			return GetInt() == Other.GetFloat();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case Float:
		switch (Other.Type)
		{
		case Float:
			return GetFloat() == Other.GetFloat();
		case Int:
			return GetFloat() == Other.GetInt();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case String:
		return GetString() == Other.GetString();

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return false;
}

/**
 * @brief Inequality operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for inequality.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the instances are not equal, false otherwise.
 */
bool ExpressoType::operator!=(const ExpressoType& Other) const
{
	return !(*this == Other);
}

/**
 * @brief Less-than operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for less-than relationship.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the current instance is less than the other instance, false otherwise.
 */
bool ExpressoType::operator<(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Undefined:
		break;

	case Bool:
		return GetBool() < Other.GetBool();
	case Int:
		switch (Other.Type)
		{
		case Int:
			return GetInt() < Other.GetInt();
		case Float:
			return GetInt() < Other.GetFloat();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case Float:
		switch (Other.Type)
		{
		case Float:
			return GetFloat() < Other.GetFloat();
		case Int:
			return GetFloat() < Other.GetInt();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case String:
		return GetString() < Other.GetString();

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return false;
}

/**
 * @brief Greater-than operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for greater-than relationship.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the current instance is greater than the other instance, false otherwise.
 */
bool ExpressoType::operator>(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Undefined:
		break;

	case Bool:
		return GetBool() > Other.GetBool();
	case Int:
		switch (Other.Type)
		{
		case Int:
			return GetInt() > Other.GetInt();
		case Float:
			return GetInt() > Other.GetFloat();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case Float:
		switch (Other.Type)
		{
		case Float:
			return GetFloat() > Other.GetFloat();
		case Int:
			return GetFloat() > Other.GetInt();
		default:
			ensureMsgf(false, TEXT("Uncomparable expresso types!"));
		}
	case String:
		return GetString() > Other.GetString();

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return false;
}

//---------------------------------------------------------------------------//

/**
 * @brief Logical AND operator for ExpressoType.
 *
 * This operator performs logical AND operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the logical AND operation.
 */
ExpressoType ExpressoType::operator&&(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Bool:
		return ExpressoType(GetBool() && Other.GetBool());
	case Int:
		return ExpressoType(GetInt() && Other.GetInt());
	case Float:
		return ExpressoType(GetFloat() && Other.GetFloat());

	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Logical OR operator for ExpressoType.
 *
 * This operator performs logical OR operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the logical OR operation.
 */
ExpressoType ExpressoType::operator||(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Bool:
		return ExpressoType(GetBool() || Other.GetBool());
	case Int:
		return ExpressoType(GetInt() || Other.GetInt());
	case Float:
		return ExpressoType(GetFloat() || Other.GetFloat());

	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Logical XOR operator for ExpressoType.
 *
 * This operator performs logical XOR operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the logical XOR operation.
 */
ExpressoType ExpressoType::operator^(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Bool:
		return ExpressoType(GetBool() ^ Other.GetBool());
	case Int:
		return ExpressoType(GetInt() ^ Other.GetInt());

	case Float:
	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Addition operator for ExpressoType.
 *
 * This operator performs addition operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the addition operation.
 */
ExpressoType ExpressoType::operator+(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Undefined:
		break;

	case Bool:
		return ExpressoType(GetBool() || Other.GetBool());
	case Int:
		return ExpressoType(GetInt() + Other.GetInt());
	case Float:
		return ExpressoType(GetFloat() + Other.GetFloat());
	case String:
		return ExpressoType(GetString() + Other.GetString());

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Subtraction operator for ExpressoType.
 *
 * This operator performs subtraction operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the subtraction operation.
 */
ExpressoType ExpressoType::operator-(const ExpressoType& Other) const
{
	return *this + (-Other);
}

/**
 * @brief Multiplication operator for ExpressoType.
 *
 * This operator performs multiplication operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the multiplication operation.
 */
ExpressoType ExpressoType::operator*(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Bool:
		return *this && Other;
	case Int:
		return ExpressoType(GetInt() * Other.GetInt());
	case Float:
		return ExpressoType(GetFloat() * Other.GetFloat());

	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Division operator for ExpressoType.
 *
 * This operator performs division operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the division operation.
 */
ExpressoType ExpressoType::operator/(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Int:
		return ExpressoType(GetInt() / Other.GetInt());
	case Float:
		return ExpressoType(GetFloat() / Other.GetFloat());

	case Bool:
	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/**
 * @brief Modulo operator for ExpressoType.
 *
 * This operator performs modulo operation on two ExpressoType instances.
 *
 * @param Other The other ExpressoType instance.
 * @return The result of the modulo operation.
 */
ExpressoType ExpressoType::operator%(const ExpressoType& Other) const
{
	switch (Type)
	{
	case Int:
		return ExpressoType(GetInt() % Other.GetInt());
	case Float:
		float OutIntPart;
		return ExpressoType(FMath::Modf(GetFloat(), &OutIntPart));

	case Bool:
	case String:
	case Undefined:
		break;

	default:
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType{};
}

/*
ExpressoType& ExpressoType::operator++()
{
	*this = this + 1;
	return *this;
}

ExpressoType ExpressoType::operator++(int)
{
	auto tmp = *this;
	*this = this + 1;
	return tmp;
}

ExpressoType& ExpressoType::operator--()
{
	*this = this - 1;
	return *this;
}

ExpressoType ExpressoType::operator--(int)
{
	auto tmp = *this;
	*this = this - 1;
	return tmp;
}
*/

//---------------------------------------------------------------------------//

/**
 * @brief Retrieves the definition of the specified C++ type.
 *
 * This method returns the definition of the specified C++ type from the ExpressoType definitions map.
 *
 * @param CppType The name of the C++ type.
 * @return The definition of the specified type.
 */
const ExpressoType::Definition& ExpressoType::GetDefinition(const FName& CppType) const
{
	if (Definitions.Num() == 0)
	{
#define ADD_DEFINITION(Type) AddDefinition<Type>(#Type);

		ADD_DEFINITION(bool);

		ADD_DEFINITION(int8);
		ADD_DEFINITION(int16);
		ADD_DEFINITION(int);
		ADD_DEFINITION(int32);
		ADD_DEFINITION(int64);

		//although they have the same size as the signed counterparts, these
		//are needed because we need to match them (i.e. need to be in the map)
		ADD_DEFINITION(uint8);
		ADD_DEFINITION(uint16);
		ADD_DEFINITION(uint32);
		ADD_DEFINITION(uint64);

		ADD_DEFINITION(float);
		ADD_DEFINITION(double);

		ADD_DEFINITION(FString);
		ADD_DEFINITION(FText);
		ADD_DEFINITION(FName);

		ADD_DEFINITION(FArticyId);
	}

	auto def = Definitions.Find(CppType);
	if (def)
		return *def;

	static const auto Empty = Definition{};
	return Empty;
}

/**
 * @brief Sets the value of a property on an object.
 *
 * This method sets the value of the specified property on the given object using the ExpressoType value.
 *
 * @param Object The object containing the property.
 * @param Property The name of the property.
 */
void ExpressoType::SetValue(UArticyBaseObject* Object, FString Property) const
{
	Object = TryFeatureReroute(Object, Property);

	if (!Object)
		return;

	auto prop = Object->GetProperty(*Property);
	if (!ensure(prop))
	{
		UE_LOG(LogArticyRuntime, Warning, TEXT("Property %s not found on Object %s!"), *Property, *Object->GetName());
		return;
	}

	FString itemType;
	FName type = *prop->GetCPPType(&itemType);

	auto setter = GetDefinition(type).Setter;

	if (ensureMsgf(setter, TEXT("Property %s has unknown type %s!"), *Property, *type.ToString()))
		setter(Object, prop, *this);
}

/**
 * @brief Attempts to reroute a property to a feature object.
 *
 * This method checks if the specified property contains a dot, indicating that it is a feature property,
 * and reroutes it to the appropriate feature object if necessary.
 *
 * @param Object The object containing the property.
 * @param Property The name of the property.
 * @return The feature object, if rerouted, or the original object if not rerouted.
 */
UArticyBaseObject* ExpressoType::TryFeatureReroute(UArticyBaseObject* Object, FString& Property)
{
	if (Object)
	{
		FString feature;
		if (Property.Split(TEXT("."), &feature, &Property))
		{
			//the property contains a dot
			//take the part before the dot to extract the feature, and use it as Object to get the actual property from
			UArticyBaseFeature* Feature = Object->GetProp<UArticyBaseFeature*>(*feature);
			if (!ensure(Feature))
			{
				UE_LOG(LogArticyRuntime, Warning, TEXT("Feature %s on Object %s is null, cannot access property %s!"),
					*feature, *Object->GetName(), *Property);
				return nullptr;
			}

			return Feature;
		}
	}

	return Object;
}

/**
 * @brief Retrieves the boolean value of the ExpressoType.
 *
 * @return The boolean value.
 */
bool& ExpressoType::GetBool() { return BoolValue; }

/**
 * @brief Retrieves the boolean value of the ExpressoType.
 *
 * @return The boolean value.
 */
const bool& ExpressoType::GetBool() const { return BoolValue; }

/**
 * @brief Retrieves the int64 value of the ExpressoType.
 *
 * @return The int64 value.
 */
int64& ExpressoType::GetInt() { return IntValue; }

/**
 * @brief Retrieves the int64 value of the ExpressoType.
 *
 * @return The int64 value.
 */
const int64& ExpressoType::GetInt() const { return IntValue; }

/**
 * @brief Retrieves the double value of the ExpressoType.
 *
 * @return The double value.
 */
double& ExpressoType::GetFloat() { return FloatValue; }

/**
 * @brief Retrieves the double value of the ExpressoType.
 *
 * @return The double value.
 */
const double& ExpressoType::GetFloat() const { return FloatValue; }

/**
 * @brief Retrieves the string value of the ExpressoType.
 *
 * @return The string value.
 */
FString& ExpressoType::GetString() { return StringValue; }

/**
 * @brief Retrieves the string value of the ExpressoType.
 *
 * @return The string value.
 */
const FString& ExpressoType::GetString() const { return StringValue; }

/**
 * @brief Constructs an ExpressoType from an int32 value.
 *
 * @param Value The int32 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const int32& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from an int16 value.
 *
 * @param Value The int16 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const int16& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from an int8 value.
 *
 * @param Value The int8 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const int8& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from a uint64 value.
 *
 * @param Value The uint64 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const uint64& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from a uint32 value.
 *
 * @param Value The uint32 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const uint32& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from a uint16 value.
 *
 * @param Value The uint16 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const uint16& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from a uint8 value.
 *
 * @param Value The uint8 value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const uint8& Value) : ExpressoType(int64(Value)) {}

/**
 * @brief Constructs an ExpressoType from a float value.
 *
 * @param Value The float value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const float& Value) : ExpressoType(double(Value)) {}

/**
 * @brief Constructs an ExpressoType from an FText value.
 *
 * @param Value The FText value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const FText& Value) : ExpressoType(Value.ToString()) {}

/**
 * @brief Constructs an ExpressoType from an FName value.
 *
 * @param Value The FName value to initialize the ExpressoType with.
 */
ExpressoType::ExpressoType(const FName& Value) : ExpressoType(Value.ToString()) {}

/**
 * @brief Conversion operator to int8.
 *
 * This operator converts the ExpressoType instance to an int8 value.
 *
 * @return The int8 value of the ExpressoType.
 */
ExpressoType::operator int8() const { return int64(*this); }

/**
 * @brief Conversion operator to uint8.
 *
 * This operator converts the ExpressoType instance to a uint8 value.
 *
 * @return The uint8 value of the ExpressoType.
 */
ExpressoType::operator uint8() const { return int64(*this); }

/**
 * @brief Conversion operator to int16.
 *
 * This operator converts the ExpressoType instance to an int16 value.
 *
 * @return The int16 value of the ExpressoType.
 */
ExpressoType::operator int16() const { return int64(*this); }

/**
 * @brief Conversion operator to uint16.
 *
 * This operator converts the ExpressoType instance to a uint16 value.
 *
 * @return The uint16 value of the ExpressoType.
 */
ExpressoType::operator uint16() const { return int64(*this); }

/**
 * @brief Conversion operator to int32.
 *
 * This operator converts the ExpressoType instance to an int32 value.
 *
 * @return The int32 value of the ExpressoType.
 */
ExpressoType::operator int32() const { return int64(*this); }

/**
 * @brief Conversion operator to uint32.
 *
 * This operator converts the ExpressoType instance to a uint32 value.
 *
 * @return The uint32 value of the ExpressoType.
 */
ExpressoType::operator uint32() const { return int64(*this); }

/**
 * @brief Conversion operator to uint64.
 *
 * This operator converts the ExpressoType instance to a uint64 value.
 *
 * @return The uint64 value of the ExpressoType.
 */
ExpressoType::operator uint64() const { return int64(*this); }

/**
 * @brief Conversion operator to float.
 *
 * This operator converts the ExpressoType instance to a float value.
 *
 * @return The float value of the ExpressoType.
 */
ExpressoType::operator float() const { return double(*this); }

/**
 * @brief Conversion operator to FText.
 *
 * This operator converts the ExpressoType instance to an FText.
 *
 * @return The FText value of the ExpressoType.
 */
ExpressoType::operator FText() const { return FText::FromString(FString(*this)); }

/**
 * @brief Conversion operator to FName.
 *
 * This operator converts the ExpressoType instance to an FName.
 *
 * @return The FName value of the ExpressoType.
 */
ExpressoType::operator FName() const { return *FString(*this); }

/**
 * @brief Conversion operator to FArticyId.
 *
 * This operator converts the ExpressoType instance to an FArticyId.
 *
 * @return The FArticyId value of the ExpressoType.
 */
ExpressoType::operator FArticyId() const { return int64(*this); }

/**
 * @brief Less-than or equal-to operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for less-than or equal-to relationship.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the current instance is less than or equal to the other instance, false otherwise.
 */
bool ExpressoType::operator<=(const ExpressoType& Other) const { return !(*this > Other); }

/**
 * @brief Greater-than or equal-to operator for ExpressoType.
 *
 * This operator compares two ExpressoType instances for greater-than or equal-to relationship.
 *
 * @param Other The other ExpressoType instance to compare.
 * @return True if the current instance is greater than or equal to the other instance, false otherwise.
 */
bool ExpressoType::operator>=(const ExpressoType& Other) const { return !(*this < Other); }

//---------------------------------------------------------------------------//

/**
 * @brief Evaluates a condition fragment.
 *
 * This method evaluates a condition fragment using the specified global variables and method provider.
 *
 * @param ConditionFragmentHash The hash of the condition fragment.
 * @param GV The global variables used in the evaluation.
 * @param MethodProvider The method provider used in the evaluation.
 * @return The result of the evaluation.
 */
bool UArticyExpressoScripts::Evaluate(const int& ConditionFragmentHash, UArticyGlobalVariables* GV,
	UObject* MethodProvider) const
{
	SetGV(GV);
	UserMethodsProvider = MethodProvider;

	auto condition = Conditions.Find(ConditionFragmentHash);
	bool result = ensure(condition) && (*condition)();

	// Clear methods provider
	UserMethodsProvider = nullptr;
	SetGV(nullptr);
	return result;
}

/**
 * @brief Executes an instruction fragment.
 *
 * This method executes an instruction fragment using the specified global variables and method provider.
 *
 * @param InstructionFragmentHash The hash of the instruction fragment.
 * @param GV The global variables used in the execution.
 * @param MethodProvider The method provider used in the execution.
 * @return True if the execution was successful, false otherwise.
 */
bool UArticyExpressoScripts::Execute(const int& InstructionFragmentHash, UArticyGlobalVariables* GV,
	UObject* MethodProvider) const
{
	SetGV(GV);
	UserMethodsProvider = MethodProvider;

	bool result = false;
	auto instruction = Instructions.Find(InstructionFragmentHash);
	if (ensure(instruction))
	{
		(*instruction)();
		result = true;
	}

	// Clear methods provider
	UserMethodsProvider = nullptr;
	SetGV(nullptr);
	return result;
}

/**
 * @brief Retrieves an Articy object by name or ID.
 *
 * This method returns an Articy object based on the specified name or ID and clone ID.
 *
 * @param NameOrId The name or ID of the object.
 * @param CloneId The clone ID of the object.
 * @return The Articy object.
 */
UArticyObject* UArticyExpressoScripts::getObj(const FString& NameOrId, const uint32& CloneId) const
{
	if (NameOrId.StartsWith(TEXT("0x")))
		return OwningDatabase->GetObject<UArticyObject>(FArticyId{ ArticyHelpers::HexToUint64(NameOrId) }, CloneId);
	if (NameOrId.IsNumeric())
		return OwningDatabase->GetObject<UArticyObject>(FArticyId{ FCString::Strtoui64(*NameOrId, NULL, 10) }, CloneId);

	return OwningDatabase->GetObjectByName(*NameOrId, CloneId);
}

/**
 * @brief Retrieves an Articy object by a compound ID.
 *
 * This method returns an Articy object based on a compound ID, which is a string representation of ID and clone ID.
 *
 * @param Id_CloneId The compound ID.
 * @return The Articy object.
 */
UArticyObject* UArticyExpressoScripts::getObjInternal(const ExpressoType& Id_CloneId) const
{
	//only works for strings!
	if (!ensureMsgf(Id_CloneId.Type == ExpressoType::String,
		TEXT("getObj(Id_CloneId) only works for string-ExpressoType!")))
		return nullptr;

	//parse id and cloneId from the compound id
	FString Id, CloneId;
	if (!Id_CloneId.GetString().Split("_", &Id, &CloneId))
		return nullptr;

	//finally get the object by id and cloneId
	return getObj(Id, FCString::Atoi(*CloneId));
}

/**
 * @brief Sets the default user methods provider.
 *
 * This method sets the default user methods provider for script evaluation and execution.
 *
 * @param MethodProvider The user methods provider.
 */
void UArticyExpressoScripts::SetDefaultUserMethodsProvider(UObject* MethodProvider)
{
	// Set a weak pointer to this object
	DefaultUserMethodsProvider = MethodProvider;
}

/**
 * @brief Retrieves the default user methods provider.
 *
 * This method returns the default user methods provider used for script evaluation and execution.
 *
 * @return The default user methods provider.
 */
UObject* UArticyExpressoScripts::GetDefaultUserMethodsProvider() const
{
	return DefaultUserMethodsProvider.Get();
}

/**
 * @brief Retrieves the user methods provider object.
 *
 * This method returns the current user methods provider object used for script evaluation and execution.
 *
 * @return The user methods provider object.
 */
UObject* UArticyExpressoScripts::GetUserMethodsProviderObject() const
{
	if (UserMethodsProvider)
		return UserMethodsProvider;
	if (DefaultUserMethodsProvider != nullptr && DefaultUserMethodsProvider.IsValid())
		return DefaultUserMethodsProvider.Get();
	return nullptr;
}

/**
 * @brief Prints a message to the log.
 *
 * This method prints the specified message to the log.
 *
 * @param msg The message to print.
 */
void UArticyExpressoScripts::PrintInternal(const FString& msg)
{
	UE_LOG(LogArticyRuntime, Log, TEXT("%s"), *msg);
}

/**
 * @brief Sets the value of a property on an Articy object.
 *
 * This method sets the value of the specified property on the given Articy object using the ExpressoType value.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @param Value The value to set.
 */
void UArticyExpressoScripts::setProp(UArticyBaseObject* Object, const FString& Property, const ExpressoType& Value)
{
	Value.SetValue(Object, Property);
}

/**
 * @brief Sets the value of a property on an Articy object.
 *
 * This method sets the value of the specified property on the given Articy object using the ExpressoType value.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @param Value The value to set.
 */
void UArticyExpressoScripts::setProp(const ExpressoType& Id_CloneId, const FString& Property,
	const ExpressoType& Value) const
{
	setProp(getObjInternal(Id_CloneId), Property, Value);
}

/**
 * @brief Retrieves the value of a property on an Articy object.
 *
 * This method returns the value of the specified property on the given Articy object as an ExpressoType.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @return The value of the property.
 */
ExpressoType UArticyExpressoScripts::getProp(UArticyBaseObject* Object, const FString& Property)
{
	return ExpressoType{ Object, Property };
}

/**
 * @brief Retrieves the value of a property on an Articy object.
 *
 * This method returns the value of the specified property on the given Articy object as an ExpressoType.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @return The value of the property.
 */
ExpressoType UArticyExpressoScripts::getProp(const ExpressoType& Id_CloneId, const FString& Property) const
{
	return getProp(getObjInternal(Id_CloneId), Property);
}

/**
 * @brief Generates a random integer between Min and Max.
 *
 * This method returns a random integer between the specified minimum and maximum values.
 *
 * @param Min The minimum value.
 * @param Max The maximum value.
 * @return The random integer.
 */
int UArticyExpressoScripts::random(int Min, int Max)
{
	return FMath::RandRange(Min, Max);
}

/**
 * @brief Generates a random integer between 0 and Max.
 *
 * This method returns a random integer between 0 and the specified maximum value.
 *
 * @param Max The maximum value.
 * @return The random integer.
 */
int UArticyExpressoScripts::random(int Max)
{
	return FMath::RandRange(0, Max);
}

/**
 * @brief Generates a random float between Min and Max.
 *
 * This method returns a random float between the specified minimum and maximum values.
 *
 * @param Min The minimum value.
 * @param Max The maximum value.
 * @return The random float.
 */
float UArticyExpressoScripts::random(float Min, float Max)
{
	return FMath::FRandRange(Min, Max);
}

/**
 * @brief Generates a random float between 0 and Max.
 *
 * This method returns a random float between 0 and the specified maximum value.
 *
 * @param Max The maximum value.
 * @return The random float.
 */
float UArticyExpressoScripts::random(float Max)
{
	return FMath::FRandRange(0, Max);
}

/**
 * @brief Generates a random ExpressoType between Min and Max.
 *
 * This method returns a random ExpressoType between the specified minimum and maximum values.
 *
 * @param Min The minimum value.
 * @param Max The maximum value.
 * @return The random ExpressoType.
 */
ExpressoType UArticyExpressoScripts::random(const ExpressoType& Min, const ExpressoType& Max)
{
	if (Min.Type != Max.Type)
	{
		ensureMsgf(false, TEXT("Cannot evaluate random value : Min and Max must be same type. Min %s / Max %s"),
			*Min.ToString(), *Max.ToString());
		return ExpressoType();
	}

	if (Min.Type == Min.Int)
	{
		return FMath::RandRange(Min.GetInt(), Max.GetInt());
	}

	if (Min.Type == Min.Float)
	{
		return FMath::RandRange((float)Min.GetFloat(), (float)Max.GetFloat());
	}

	if (Min.Type == Min.Bool || Min.Type == Min.String || Min.Type == Min.Undefined)
	{
		ensureMsgf(false, TEXT("Cannot evaluate random value from %s"), *Min.ToString());
	}
	else
	{
		ensureMsgf(false, TEXT("Unknown ArticyExpressoType!"));
	}

	return ExpressoType();
}

/**
 * @brief Generates a random ExpressoType between 0 and Max.
 *
 * This method returns a random ExpressoType between 0 and the specified maximum value.
 *
 * @param Max The maximum value.
 * @return The random ExpressoType.
 */
ExpressoType UArticyExpressoScripts::random(const ExpressoType& Max)
{
	return random(ExpressoType(0), Max);
}

/**
 * @brief Increments the value of a property on an Articy object.
 *
 * This method increments the value of the specified property on the given Articy object by the specified value.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @param Value The increment value (default is 1).
 */
void UArticyExpressoScripts::incrementProp(UArticyBaseObject* Object, const FString& Property,
	const float Value /*= 1*/)
{
	float curvalue = (float)getProp(Object, Property);
	setProp(Object, Property, static_cast<decltype(getProp(Object, Property))>(curvalue + Value));
}

/**
 * @brief Increments the value of a property on an Articy object.
 *
 * This method increments the value of the specified property on the given Articy object by the specified value.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @param Value The increment value (default is 1).
 */
void UArticyExpressoScripts::incrementProp(const ExpressoType& Id_CloneId, const FString& Property,
	const float Value /*= 1*/) const
{
	incrementProp(getObjInternal(Id_CloneId), Property, Value);
}

/**
 * @brief Decrements the value of a property on an Articy object.
 *
 * This method decrements the value of the specified property on the given Articy object by the specified value.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @param Value The decrement value (default is 1).
 */
void UArticyExpressoScripts::decrementProp(UArticyBaseObject* Object, const FString& Property,
	const float Value /*= 1*/)
{
	float curvalue = (float)getProp(Object, Property);
	setProp(Object, Property, static_cast<decltype(getProp(Object, Property))>(curvalue - Value));
}

/**
 * @brief Decrements the value of a property on an Articy object.
 *
 * This method decrements the value of the specified property on the given Articy object by the specified value.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @param Value The decrement value (default is 1).
 */
void UArticyExpressoScripts::decrementProp(const ExpressoType& Id_CloneId, const FString& Property,
	const float Value /*= 1*/) const
{
	decrementProp(getObjInternal(Id_CloneId), Property, Value);
}

/**
 * @brief Checks if a value is within a range.
 *
 * This method checks if the specified value is within the given lower and upper bounds.
 *
 * @param valueToTest The value to test.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isInRange(float valueToTest, float lowerBound, float upperBound)
{
	return valueToTest >= lowerBound && valueToTest <= upperBound;
}

/**
 * @brief Checks if a property value is within a range.
 *
 * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the property value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isPropInRange(UArticyBaseObject* Object, const FString& Property, float lowerBound,
	float upperBound)
{
	return isInRange((float)getProp(Object, Property), lowerBound, upperBound);
}

/**
 * @brief Checks if a property value is within a range.
 *
 * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the property value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isPropInRange(const ExpressoType& Id_CloneId, const FString& Property, float lowerBound,
	float upperBound) const
{
	return isPropInRange(getObjInternal(Id_CloneId), Property, lowerBound, upperBound);
}

/**
 * @brief Checks if a string value is within a range.
 *
 * This method checks if the specified string value is within the given lower and upper bounds.
 *
 * @param valueToTest The string value to test.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the string value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isInRange(const FString& valueToTest, const FString& lowerBound, const FString& upperBound)
{
	return valueToTest.Compare(lowerBound, ESearchCase::CaseSensitive) >= 0 && valueToTest.Compare(
		upperBound, ESearchCase::CaseSensitive) <= 0;
}

/**
 * @brief Checks if a property value is within a range.
 *
 * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
 *
 * @param Object The Articy object containing the property.
 * @param Property The name of the property.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the property value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isPropInRange(UArticyBaseObject* Object, const FString& Property,
	const FString& lowerBound, const FString& upperBound)
{
	return isInRange((FString)getProp(Object, Property), lowerBound, upperBound);
}

/**
 * @brief Checks if a property value is within a range.
 *
 * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
 *
 * @param Id_CloneId The compound ID of the object.
 * @param Property The name of the property.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 * @return True if the property value is within the range, false otherwise.
 */
bool UArticyExpressoScripts::isPropInRange(const ExpressoType& Id_CloneId, const FString& Property,
	const FString& lowerBound, const FString& upperBound) const
{
	return isPropInRange(getObjInternal(Id_CloneId), Property, lowerBound, upperBound);
}

/**
 * @brief Resets all seen counters in the global variables.
 *
 * This method resets all seen counters in the global variables.
 */
void UArticyExpressoScripts::resetAllSeenCounters()
{
	UArticyGlobalVariables* GVs = GetGV();
	if (GVs)
	{
		GVs->ResetVisited();
	}
}

/**
 * @brief Retrieves the seen counter for an Articy object.
 *
 * This method returns the seen counter for the specified Articy object, or the current object if none is specified.
 *
 * @param Object The Articy object (default is nullptr, which means the current object).
 * @return The seen counter value.
 */
int UArticyExpressoScripts::getSeenCounter(UArticyBaseObject* Object)
{
	if (Object == nullptr)
	{
		Object = self;
	}

	UArticyGlobalVariables* GVs = GetGV();
	auto* Obj = Cast<IArticyFlowObject>(Object);
	if (GVs && Obj)
	{
		return GVs->GetSeenCounter(Obj);
	}
	return 0;
}

/**
 * @brief Sets the seen counter for the current object.
 *
 * This method sets the seen counter for the current object to the specified value.
 *
 * @param Value The value to set (default is 1).
 * @return The updated seen counter value.
 */
int UArticyExpressoScripts::setSeenCounter(const int Value)
{
	return setSeenCounter(self, Value);
}

/**
 * @brief Sets the seen counter for an Articy object.
 *
 * This method sets the seen counter for the specified Articy object to the specified value.
 *
 * @param Object The Articy object.
 * @param Value The value to set.
 * @return The updated seen counter value.
 */
int UArticyExpressoScripts::setSeenCounter(UArticyBaseObject* Object, const int Value)
{
	if (Object == nullptr)
	{
		Object = self;
	}

	UArticyGlobalVariables* GVs = GetGV();
	auto* Obj = Cast<IArticyFlowObject>(Object);
	if (GVs && Obj)
	{
		return GVs->SetSeenCounter(Obj, Value);
	}
	return 0;
}

/**
 * @brief Retrieves the seen counter for an Articy object by name or ID.
 *
 * This method returns the seen counter for the specified Articy object based on its name or ID.
 *
 * @param NameOrId The name or ID of the object.
 * @return The seen counter value.
 */
int UArticyExpressoScripts::getSeenCounter(const FString& NameOrId)
{
	return getSeenCounter(getObj(NameOrId, 0));
}

/**
 * @brief Sets the seen counter for an Articy object by name or ID.
 *
 * This method sets the seen counter for the specified Articy object based on its name or ID to the specified value.
 *
 * @param NameOrId The name or ID of the object.
 * @param Value The value to set.
 * @return The updated seen counter value.
 */
int UArticyExpressoScripts::setSeenCounter(const FString& NameOrId, const int Value)
{
	return setSeenCounter(getObj(NameOrId, 0), Value);
}

/**
 * @brief Performs a fallback operation for an Articy object.
 *
 * This method performs a fallback operation for the specified Articy object.
 *
 * @param Object The Articy object.
 * @return True if the fallback operation was successful, false otherwise.
 */
bool UArticyExpressoScripts::fallback(UArticyBaseObject* Object)
{
	UArticyGlobalVariables* GVs = GetGV();
	if (Object)
	{
		auto* Obj = Cast<IArticyFlowObject>(Object);
		if (GVs && Obj)
		{
			return GVs->Fallback(Obj);
		}
	}
	return GVs->Fallback(nullptr);
}

/**
 * @brief Performs a fallback operation for an Articy object by name or ID.
 *
 * This method performs a fallback operation for the specified Articy object based on its name or ID.
 *
 * @param NameOrId The name or ID of the object.
 * @return True if the fallback operation was successful, false otherwise.
 */
bool UArticyExpressoScripts::fallback(const FString& NameOrId)
{
	return fallback(getObj(NameOrId, 0));
}

/**
 * @brief Performs a fallback operation for the current object.
 *
 * This method performs a fallback operation for the current object.
 *
 * @return True if the fallback operation was successful, false otherwise.
 */
bool UArticyExpressoScripts::fallback()
{
	return fallback(nullptr);
}
