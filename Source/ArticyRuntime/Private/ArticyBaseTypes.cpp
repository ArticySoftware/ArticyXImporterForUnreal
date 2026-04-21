//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyBaseTypes.h"
#include "ArticyHelpers.h"
#include "ArticyDatabase.h"

/**
 * Initializes the FArticyId from a given string containing "Low=" and "High=".
 *
 * @param InSourceString The string from which to initialize the ID.
 * @return true if initialization is successful, otherwise false.
 */
bool FArticyId::InitFromString(const FString InSourceString)
{
	Low = High = 0;

	const bool bSuccessful = FParse::Value(*InSourceString, TEXT("Low="), Low) && FParse::Value(*InSourceString, TEXT("High="), High);

	return bSuccessful;
}

/**
 * Retrieves the UArticyPrimitive object associated with this ID.
 *
 * @param WorldContext The world context object.
 * @return A pointer to the UArticyPrimitive object if found, otherwise nullptr.
 */
UArticyPrimitive* FArticyId::GetObject(const UObject* WorldContext) const
{
	auto db = UArticyDatabase::Get(WorldContext);
	return db ? db->GetObject(*this) : nullptr;
}

/**
 * Constructs an FArticySize from a JSON value.
 *
 * @param Json The JSON value to construct the size from.
 */
FArticySize::FArticySize(TSharedPtr<FJsonValue> Json)
{
	if (!ensure(Json.IsValid() && Json->Type == EJson::Object))
		return;

	auto obj = Json->AsObject();
	w = obj->GetNumberField(TEXT(STRINGIFY(w)));
	h = obj->GetNumberField(TEXT(STRINGIFY(h)));
}

/**
 * Constructs an FArticyRect from a JSON value.
 *
 * @param Json The JSON value to construct the rectangle from.
 */
FArticyRect::FArticyRect(TSharedPtr<FJsonValue> Json)
{
	if (!ensure(Json.IsValid() && Json->Type == EJson::Object))
		return;

	auto obj = Json->AsObject();

	x = obj->GetNumberField(TEXT(STRINGIFY(x)));
	y = obj->GetNumberField(TEXT(STRINGIFY(y)));
	w = obj->GetNumberField(TEXT(STRINGIFY(w)));
	h = obj->GetNumberField(TEXT(STRINGIFY(h)));
}

/**
 * Constructs an FArticyStdString from a JSON value.
 *
 * @param Json The JSON value to construct the string from.
 */
FArticyStdString::FArticyStdString(const TSharedPtr<FJsonValue>& Json)
{
	if (!ensure(Json.IsValid() && Json->Type == EJson::String))
		return;

	Text = FText::AsCultureInvariant(Json->AsString());
}
