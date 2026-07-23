//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "ArticyFlowClasses.h"
#include "ArticyBuiltinTypes.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyPropertyDef MakePropertyDef(const FString& Property, const FString& Type)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Property"), Property);
		Json->SetStringField(TEXT("Type"), Type);

		UArticyImportData* Data = NewObject<UArticyImportData>();
		FArticyPropertyDef Def;
		Def.ImportFromJson(Json, Data);
		return Def;
	}
}

// The three lookups here decide what generated code an object definition turns into: which
// engine class it derives from, which IArticyObjectWith* interface it implements, and what
// a property is initialised to. Each is a hard-coded table, so an entry that goes missing
// removes an interface from every generated class that needs it - which shows up as a build
// error in the generated module, or as a runtime cast that quietly starts failing.
BEGIN_DEFINE_SPEC(FArticyObjectDefLookupSpec, "Articy.Editor.ObjectDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyObjectDefLookupSpec)

void FArticyObjectDefLookupSpec::Define()
{
	Describe("GetDefaultBaseClass", [this]()
	{
		It("maps each flow type onto its runtime class", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			auto BaseOf = [Data](const TCHAR* Type)
			{
				return FArticyObjectDefinitions::GetDefaultBaseClass(FName(Type), Data);
			};

			TestEqual(TEXT("Dialogue"), BaseOf(TEXT("Dialogue")).CppTypeName, FString(TEXT("UArticyDialogue")));
			TestEqual(TEXT("DialogueFragment"), BaseOf(TEXT("DialogueFragment")).CppTypeName,
				FString(TEXT("UArticyDialogueFragment")));
			TestEqual(TEXT("FlowFragment"), BaseOf(TEXT("FlowFragment")).CppTypeName, FString(TEXT("UArticyFlowFragment")));
			TestEqual(TEXT("Hub"), BaseOf(TEXT("Hub")).CppTypeName, FString(TEXT("UArticyHub")));
			TestEqual(TEXT("Jump"), BaseOf(TEXT("Jump")).CppTypeName, FString(TEXT("UArticyJump")));
			TestEqual(TEXT("Condition"), BaseOf(TEXT("Condition")).CppTypeName, FString(TEXT("UArticyCondition")));
			TestEqual(TEXT("Instruction"), BaseOf(TEXT("Instruction")).CppTypeName, FString(TEXT("UArticyInstruction")));
			TestEqual(TEXT("Entity"), BaseOf(TEXT("Entity")).CppTypeName, FString(TEXT("UArticyEntity")));
			TestEqual(TEXT("Asset"), BaseOf(TEXT("Asset")).CppTypeName, FString(TEXT("UArticyAsset")));
			TestEqual(TEXT("Location"), BaseOf(TEXT("Location")).CppTypeName, FString(TEXT("UArticyLocation")));
			TestEqual(TEXT("Zone"), BaseOf(TEXT("Zone")).CppTypeName, FString(TEXT("UArticyZone")));
		});

		It("resolves the reflected UClass alongside the type name", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			const FArticyObjectDefinitions::FClassInfo& Info =
				FArticyObjectDefinitions::GetDefaultBaseClass(FName(TEXT("Dialogue")), Data);
			TestTrue(TEXT("class matches"), Info.StaticClass == UArticyDialogue::StaticClass());
		});

		It("falls back to UArticyObject for anything else", [this]()
		{
			// Templates and unknown types are not flow classes and derive from the plain
			// object base.
			UArticyImportData* Data = NewObject<UArticyImportData>();
			const FArticyObjectDefinitions::FClassInfo& Info =
				FArticyObjectDefinitions::GetDefaultBaseClass(FName(TEXT("MyNpcTemplate")), Data);
			TestEqual(TEXT("type name"), Info.CppTypeName, FString(TEXT("UArticyObject")));
			TestTrue(TEXT("class"), Info.StaticClass == UArticyObject::StaticClass());
		});
	});

	Describe("GetProviderInterface", [this]()
	{
		It("maps a well-known property onto its IArticyObjectWith* interface", [this]()
		{
			TestEqual(TEXT("Text"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("Text"), TEXT("string"))).ToString(),
				FString(TEXT("IArticyObjectWithText")));
			TestEqual(TEXT("DisplayName"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("DisplayName"), TEXT("string"))).ToString(),
				FString(TEXT("IArticyObjectWithDisplayName")));
			TestEqual(TEXT("Speaker"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("Speaker"), TEXT("id"))).ToString(),
				FString(TEXT("IArticyObjectWithSpeaker")));
			TestEqual(TEXT("Position"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("Position"), TEXT("point"))).ToString(),
				FString(TEXT("IArticyObjectWithPosition")));
			TestEqual(TEXT("PreviewImage"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("PreviewImage"), TEXT("PreviewImage"))).ToString(),
				FString(TEXT("IArticyObjectWithPreviewImage")));
			TestEqual(TEXT("Target"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("Target"), TEXT("id"))).ToString(),
				FString(TEXT("IArticyObjectWithTarget")));
		});

		It("covers every interface the runtime ships", [this]()
		{
			// Kept as an explicit list so adding an IArticyObjectWith* interface without
			// registering its property here shows up as a failure rather than as a
			// generated class that silently does not implement it.
			const TArray<FString> Expected{
				TEXT("Attachments"), TEXT("Color"), TEXT("DisplayName"), TEXT("ExternalId"),
				TEXT("MenuText"), TEXT("Position"), TEXT("PreviewImage"), TEXT("ShortId"),
				TEXT("Size"), TEXT("Speaker"), TEXT("StageDirections"), TEXT("Target"),
				TEXT("Text"), TEXT("Transform"), TEXT("Vertices"), TEXT("ZIndex")
			};

			for (const FString& Property : Expected)
			{
				const FName Interface =
					FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(Property, TEXT("string")));
				TestEqual(*Property, Interface.ToString(), FString(TEXT("IArticyObjectWith")) + Property);
			}
		});

		It("returns None for a property with no provider interface", [this]()
		{
			// A template's own properties get no interface; only the built-in ones do.
			TestTrue(TEXT("no interface"),
				FArticyObjectDefinitions::GetProviderInterface(MakePropertyDef(TEXT("HP"), TEXT("int"))).IsNone());
		});
	});

	Describe("GetCppType / GetCppDefaultValue", [this]()
	{
		It("resolves a predefined type by value and by property", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDefinitions Defs;

			TestEqual(TEXT("int as value"), Defs.GetCppType(FName(TEXT("int")), Data, false), FString(TEXT("int32")));
			TestEqual(TEXT("int as property"), Defs.GetCppType(FName(TEXT("int")), Data, true), FString(TEXT("int32")));
			// Articy objects differ between the two: the property form is a pointer.
			TestEqual(TEXT("pin as value"), Defs.GetCppType(FName(TEXT("InputPin")), Data, false),
				FString(TEXT("UArticyInputPin")));
			TestEqual(TEXT("pin as property"), Defs.GetCppType(FName(TEXT("InputPin")), Data, true),
				FString(TEXT("UArticyInputPin*")));
		});

		It("returns the predefined default value", [this]()
		{
			TestEqual(TEXT("int"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("int"))), FString(TEXT("0")));
			TestEqual(TEXT("bool"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("bool"))), FString(TEXT("false")));
			TestEqual(TEXT("float"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("float"))), FString(TEXT("0.f")));
			TestEqual(TEXT("point"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("point"))),
				FString(TEXT("FVector2D::ZeroVector")));
			TestEqual(TEXT("color"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("color"))),
				FString(TEXT("FLinearColor::Black")));
			TestEqual(TEXT("InputPin"), FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("InputPin"))),
				FString(TEXT("nullptr")));
		});

		It("returns an empty default for a non-predefined type", [this]()
		{
			// Generated classes have no literal default; the property is left to its own
			// constructor.
			TestTrue(TEXT("empty"),
				FArticyObjectDefinitions::GetCppDefaultValue(FName(TEXT("MyNpcTemplate"))).IsEmpty());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
