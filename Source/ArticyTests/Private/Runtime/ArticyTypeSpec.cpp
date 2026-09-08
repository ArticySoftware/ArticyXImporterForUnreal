//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyType.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyEnumValueInfo MakeEnumValue(const FString& TechnicalName, int Value)
	{
		FArticyEnumValueInfo Info;
		Info.TechnicalName = TechnicalName;
		Info.DisplayName = TechnicalName;
		Info.LocaKey_DisplayName = TechnicalName;
		Info.Value = Value;
		return Info;
	}

	// The importer fills both names: lookups go by technical name, the loca key is what
	// gets handed to the string table. They are kept distinct here so a lookup that
	// matched the wrong one would fail the test.
	FArticyPropertyInfo MakeProperty(const FString& TechnicalName, const FString& Type)
	{
		FArticyPropertyInfo Info;
		Info.TechnicalName = TechnicalName;
		Info.LocaKey_DisplayName = TechnicalName + TEXT(".DisplayName");
		Info.PropertyType = Type;
		return Info;
	}

	FArticyPropertyInfo MakeFeatureProperty(const FString& Feature, const FString& TechnicalName, const FString& Type)
	{
		FArticyPropertyInfo Info = MakeProperty(Feature + TEXT(".") + TechnicalName, Type);
		Info.IsTemplateProperty = true;
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
			TestEqual(TEXT("by value"), Type.GetEnumValue(1).TechnicalName, FString(TEXT("Green")));
		});

		It("finds an enum value by its technical name", [this]()
		{
			FArticyType Type;
			Type.EnumValues = { MakeEnumValue(TEXT("Red"), 0), MakeEnumValue(TEXT("Green"), 1) };
			TestEqual(TEXT("by name"), Type.GetEnumValue(FString(TEXT("Green"))).Value, 1);
		});

		It("returns a default value when nothing matches", [this]()
		{
			FArticyType Type;
			Type.EnumValues = { MakeEnumValue(TEXT("Red"), 0) };
			TestEqual(TEXT("missing"), Type.GetEnumValue(99).TechnicalName, FString());
		});
	});

	Describe("GetProperty", [this]()
	{
		It("finds a property by its technical name", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeProperty(TEXT("Speed"), TEXT("float")) };
			TestEqual(TEXT("type"), Type.GetProperty(TEXT("Speed")).PropertyType, FString(TEXT("float")));
		});

		It("finds a feature property by its qualified name", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeFeatureProperty(TEXT("Stats"), TEXT("Speed"), TEXT("float")) };
			TestEqual(TEXT("qualified"), Type.GetProperty(TEXT("Stats.Speed")).PropertyType, FString(TEXT("float")));
			TestTrue(TEXT("unqualified does not match"), Type.GetProperty(TEXT("Speed")).PropertyType.IsEmpty());
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

		It("returns the properties belonging to a feature", [this]()
		{
			FArticyType Type;
			Type.Features = { TEXT("Stats") };
			Type.Properties = {
				MakeProperty(TEXT("DisplayName"), TEXT("string")),
				MakeFeatureProperty(TEXT("Stats"), TEXT("Speed"), TEXT("float")),
				MakeFeatureProperty(TEXT("Stats"), TEXT("Health"), TEXT("int")),
				MakeFeatureProperty(TEXT("Combat"), TEXT("Damage"), TEXT("int"))
			};

			const TArray<FArticyPropertyInfo> InFeature = Type.GetPropertiesInFeature(TEXT("Stats"));

			TestEqual(TEXT("count"), InFeature.Num(), 2);
			if (InFeature.Num() == 2)
			{
				// Names stay qualified, so they can be fed straight back into GetProperty.
				TestEqual(TEXT("first"), InFeature[0].TechnicalName, FString(TEXT("Stats.Speed")));
				TestEqual(TEXT("second"), InFeature[1].TechnicalName, FString(TEXT("Stats.Health")));
			}
		});

		It("returns nothing for a feature that has no properties", [this]()
		{
			FArticyType Type;
			Type.Properties = { MakeFeatureProperty(TEXT("Stats"), TEXT("Speed"), TEXT("float")) };
			TestEqual(TEXT("empty"), Type.GetPropertiesInFeature(TEXT("Combat")).Num(), 0);
		});

		It("does not mistake a plain property for a feature property", [this]()
		{
			// A non-template property whose name happens to carry a dot must not be
			// reported as living in a feature.
			FArticyType Type;
			Type.Properties = { MakeProperty(TEXT("Stats.Speed"), TEXT("float")) };
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
