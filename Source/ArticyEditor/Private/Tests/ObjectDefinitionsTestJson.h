//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

// Shared JSON builders for the object-definition specs, which each cover one type.
namespace ArticyObjectDefTestJson
{
	// Wraps a single JSON object into a one-element JSON array
	inline TArray<TSharedPtr<FJsonValue>> OneObject(const TSharedPtr<FJsonObject>& Obj)
	{
		TArray<TSharedPtr<FJsonValue>> Array;
		Array.Add(MakeShared<FJsonValueObject>(Obj));
		return Array;
	}

	inline TSharedPtr<FJsonObject> PropJson(const FString& Property, const FString& Type)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Property"), Property);
		Json->SetStringField(TEXT("Type"), Type);
		return Json;
	}

	inline TSharedPtr<FJsonObject> ConstraintJson(const FString& Property, const FString& Type, bool bLocalized)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Property"), Property);
		Json->SetStringField(TEXT("Type"), Type);
		Json->SetBoolField(TEXT("IsLocalized"), bLocalized);
		return Json;
	}

	// A feature with a single "HP" int property and a matching constraint
	inline TSharedPtr<FJsonObject> FeatureJson(const FString& TechnicalName, const FString& DisplayName)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("TechnicalName"), TechnicalName);
		Json->SetStringField(TEXT("DisplayName"), DisplayName);
		Json->SetArrayField(TEXT("Constraints"), OneObject(ConstraintJson(TEXT("HP"), TEXT("int"), false)));
		Json->SetArrayField(TEXT("Properties"), OneObject(PropJson(TEXT("HP"), TEXT("int"))));
		return Json;
	}

	// A template carrying one feature
	inline TSharedPtr<FJsonObject> TemplateJson(const FString& TechnicalName, const FString& DisplayName)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("TechnicalName"), TechnicalName);
		Json->SetStringField(TEXT("DisplayName"), DisplayName);
		Json->SetArrayField(TEXT("Features"), OneObject(FeatureJson(TEXT("Stats"), TEXT("Stats Feature"))));
		return Json;
	}
}

#endif // WITH_AUTOMATION_TESTS
