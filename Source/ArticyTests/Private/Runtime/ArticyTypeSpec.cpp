//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyType.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyEnumValueInfo MakeEnumValue(const FString& LocaKey, int Value)
	{
		FArticyEnumValueInfo Info;
		Info.LocaKey_DisplayName = LocaKey;
		Info.Value = Value;
		return Info;
	}

	FArticyPropertyInfo MakeProperty(const FString& LocaKey, const FString& Type)
	{
		FArticyPropertyInfo Info;
		Info.LocaKey_DisplayName = LocaKey;
		Info.PropertyType = Type;
		return Info;
	}
}

BEGIN_DEFINE_SPEC(FArticyTypeSpec, "Articy.Runtime.Type",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTypeSpec)

void FArticyTypeSpec::Define()
{
	Describe("GetEnumValue", [this]()
	{
		It("finds an enum value by its integer value", [this]()
		{
			FArticyType Type;
			Type.EnumValues = { MakeEnumValue(TEXT("Red"), 0), MakeEnumValue(TEXT("Green"), 1) };
			TestEqual(TEXT("by value"), Type.GetEnumValue(1).LocaKey_DisplayName, FString(TEXT("Green")));
		});

		It("finds an enum value by its loca key", [this]()
		{
			FArticyType Type;
			Type.EnumValues = { MakeEnumValue(TEXT("Red"), 0), MakeEnumValue(TEXT("Green"), 1) };
			TestEqual(TEXT("by name"), Type.GetEnumValue(FString(TEXT("Green"))).Value, 1);
		});

		It("returns a default value when nothing matches", [this]()
		{
			FArticyType Type;
			Type.EnumValues = { MakeEnumValue(TEXT("Red"), 0) };
			TestEqual(TEXT("missing"), Type.GetEnumValue(99).LocaKey_DisplayName, FString());
		});
	});

	Describe("GetProperty", [this]()
	{
		It("finds a property by its loca key", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeProperty(TEXT("Speed"), TEXT("float")) };
			TestEqual(TEXT("type"), Type.GetProperty(TEXT("Speed")).PropertyType, FString(TEXT("float")));
		});

		It("returns a default property when nothing matches", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeProperty(TEXT("Speed"), TEXT("float")) };
			TestTrue(TEXT("missing"), Type.GetProperty(TEXT("Unknown")).PropertyType.IsEmpty());
		});
	});

	Describe("MergeParent", [this]()
	{
		It("fills empty fields from the parent but keeps existing ones", [this]()
		{
			FArticyType Parent;
			Parent.CPPType = TEXT("ParentType");
			Parent.TechnicalName = TEXT("ParentName");
			Parent.HasTemplate = true;

			FArticyType Child;
			Child.CPPType = TEXT("ChildType");

			Child.MergeParent(Parent);

			TestEqual(TEXT("keeps own CPPType"), Child.CPPType, FString(TEXT("ChildType")));
			TestEqual(TEXT("inherits TechnicalName"), Child.TechnicalName, FString(TEXT("ParentName")));
			TestTrue(TEXT("inherits HasTemplate"), Child.HasTemplate);
		});
	});

	Describe("Feature helpers", [this]()
	{
		It("returns the feature name unchanged as its loca key", [this]()
		{
			FArticyType Type;
			TestEqual(TEXT("loca key"), Type.GetFeatureDisplayNameLocaKey(TEXT("Stats")), FString(TEXT("Stats")));
		});

		It("returns no properties for a feature (not yet implemented)", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeProperty(TEXT("Speed"), TEXT("float")) };
			TestEqual(TEXT("empty"), Type.GetPropertiesInFeature(TEXT("Stats")).Num(), 0);
		});
	});

	Describe("MergeChild", [this]()
	{
		It("overrides fields with the child's non-empty values", [this]()
		{
			FArticyType Type;
			Type.CPPType = TEXT("BaseType");
			Type.TechnicalName = TEXT("BaseName");

			FArticyType Child;
			Child.CPPType = TEXT("ChildType");
			// TechnicalName left empty: should not overwrite.

			Type.MergeChild(Child);

			TestEqual(TEXT("overridden CPPType"), Type.CPPType, FString(TEXT("ChildType")));
			TestEqual(TEXT("kept TechnicalName"), Type.TechnicalName, FString(TEXT("BaseName")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
