//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyTypeSystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyType MakeCharacterType()
	{
		FArticyPropertyInfo DisplayName;
		DisplayName.TechnicalName = TEXT("DisplayName");
		DisplayName.PropertyType = TEXT("string");

		FArticyType Type;
		Type.TechnicalName = TEXT("Character");
		Type.CPPType = TEXT("UTestProjectCharacter");
		Type.Properties = { DisplayName };
		return Type;
	}
}

BEGIN_DEFINE_SPEC(FArticyTypeSystemSpec, "Articy.Runtime.TypeSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTypeSystemSpec)

void FArticyTypeSystemSpec::Define()
{
	Describe("GetArticyType", [this]()
	{
		It("returns the metadata registered for a type name", [this]()
		{
			TStrongObjectPtr<UArticyTypeSystem> TypeSystem(NewObject<UArticyTypeSystem>());
			TypeSystem->Types.Add(TEXT("Character"), MakeCharacterType());

			const FArticyType Type = TypeSystem->GetArticyType(TEXT("Character"));
			TestEqual(TEXT("technical name"), Type.TechnicalName, FString(TEXT("Character")));
			TestEqual(TEXT("property type"), Type.GetProperty(TEXT("DisplayName")).PropertyType, FString(TEXT("string")));
		});

		It("returns a default type for an unknown type name", [this]()
		{
			TStrongObjectPtr<UArticyTypeSystem> TypeSystem(NewObject<UArticyTypeSystem>());
			TypeSystem->Types.Add(TEXT("Character"), MakeCharacterType());

			TestTrue(TEXT("unknown"), TypeSystem->GetArticyType(TEXT("Nope")).TechnicalName.IsEmpty());
		});

		It("returns a default type when nothing has been imported", [this]()
		{
			TStrongObjectPtr<UArticyTypeSystem> TypeSystem(NewObject<UArticyTypeSystem>());
			TestTrue(TEXT("empty"), TypeSystem->GetArticyType(TEXT("Character")).TechnicalName.IsEmpty());
		});
	});

	Describe("Get", [this]()
	{
		It("always returns a usable instance", [this]()
		{
			// Without an imported project there is no generated type system asset, so this
			// falls back to an empty placeholder rather than returning null.
			TestNotNull(TEXT("instance"), UArticyTypeSystem::Get());
		});

		It("returns the same instance while it is alive", [this]()
		{
			TStrongObjectPtr<UArticyTypeSystem> First(UArticyTypeSystem::Get());
			TestTrue(TEXT("cached"), UArticyTypeSystem::Get() == First.Get());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
