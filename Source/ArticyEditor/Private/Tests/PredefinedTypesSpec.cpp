//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "PredefinedTypes.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyPluginSettings.h"
#include "ArticyTestTextModel.h"
#include "Dom/JsonValue.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyPredefinedTypesSpec, "Articy.Editor.PredefinedTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPredefinedTypesSpec)

void FArticyPredefinedTypesSpec::Define()
{
	Describe("DecodeHtmlEntities", [this]()
	{
		It("decodes named entities", [this]()
		{
			TestEqual(TEXT("lt"), DecodeHtmlEntities(TEXT("a &lt; b")), FString(TEXT("a < b")));
			TestEqual(TEXT("gt"), DecodeHtmlEntities(TEXT("a &gt; b")), FString(TEXT("a > b")));
			TestEqual(TEXT("amp"), DecodeHtmlEntities(TEXT("&amp;")), FString(TEXT("&")));
		});

		It("decodes decimal and hex numeric entities", [this]()
		{
			TestEqual(TEXT("decimal"), DecodeHtmlEntities(TEXT("&#65;")), FString(TEXT("A")));
			TestEqual(TEXT("hex"), DecodeHtmlEntities(TEXT("&#x41;")), FString(TEXT("A")));
		});

		It("leaves plain text unchanged", [this]()
		{
			TestEqual(TEXT("plain"), DecodeHtmlEntities(TEXT("hello world")), FString(TEXT("hello world")));
		});
	});

	Describe("ConvertUnityMarkupToUnreal", [this]()
	{
		It("converts a Unity tag pair to Unreal markup", [this]()
		{
			TestEqual(TEXT("bold"), ConvertUnityMarkupToUnreal(TEXT("<b>Hello</b>")), FString(TEXT("<b>Hello</>")));
		});

		It("drops the align tag (unsupported in Unreal)", [this]()
		{
			TestEqual(TEXT("align dropped"), ConvertUnityMarkupToUnreal(TEXT("<align=left>Hi</align>")), FString(TEXT("Hi")));
		});

		It("leaves text without tags unchanged", [this]()
		{
			TestEqual(TEXT("no tags"), ConvertUnityMarkupToUnreal(TEXT("just text")), FString(TEXT("just text")));
		});
	});

	// The predefined-type table maps an articy export type onto the C++ type, property type
	// and default value that end up verbatim in the generated headers. A wrong or missing
	// entry produces code that does not compile, or silently changes an imported property's
	// type, so the entries the generator depends on are pinned here.
	Describe("FArticyPredefTypes", [this]()
	{
		const auto TypeInfo = [](const TCHAR* OriginalType)
		{
			FArticyPredefinedTypeBase** Found = FArticyPredefTypes::Get().Find(FName(OriginalType));
			return Found ? *Found : nullptr;
		};

		It("recognises the scalar export types", [this]()
		{
			TestTrue(TEXT("int"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("int"))));
			TestTrue(TEXT("uint"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("uint"))));
			TestTrue(TEXT("float"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("float"))));
			TestTrue(TEXT("bool"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("bool"))));
			TestTrue(TEXT("boolean"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("boolean"))));
			TestTrue(TEXT("string"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("string"))));
			TestTrue(TEXT("id"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("id"))));
			TestTrue(TEXT("DateTime"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("DateTime"))));
		});

		It("recognises the composite and articy object types", [this]()
		{
			TestTrue(TEXT("point"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("point"))));
			TestTrue(TEXT("rect"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("rect"))));
			TestTrue(TEXT("size"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("size"))));
			TestTrue(TEXT("color"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("color"))));
			TestTrue(TEXT("array"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("array"))));
			TestTrue(TEXT("InputPin"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("InputPin"))));
			TestTrue(TEXT("OutputPin"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("OutputPin"))));
			TestTrue(TEXT("IncomingConnection"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("IncomingConnection"))));
			TestTrue(TEXT("OutgoingConnection"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("OutgoingConnection"))));
			TestTrue(TEXT("Script_Condition"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("Script_Condition"))));
			TestTrue(TEXT("Script_Instruction"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("Script_Instruction"))));
			TestTrue(TEXT("Transformation"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("Transformation"))));
		});

		It("does not recognise a template type", [this]()
		{
			// Template-defined types are generated, not predefined; treating one as
			// predefined would skip generating its class.
			TestFalse(TEXT("template"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("MyNpcTemplate"))));
		});

		It("looks types up case-insensitively", [this]()
		{
			// The table is keyed by FName, and the export's casing has changed before.
			TestTrue(TEXT("INT"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("INT"))));
			TestTrue(TEXT("inputpin"), FArticyPredefTypes::IsPredefinedType(FName(TEXT("inputpin"))));
		});

		It("declares scalars by value with a compilable default", [this, TypeInfo]()
		{
			FArticyPredefinedTypeBase* Int = TypeInfo(TEXT("int"));
			if (!TestNotNull(TEXT("int entry"), Int)) return;
			TestEqual(TEXT("cpp type"), Int->CppType, FString(TEXT("int32")));
			TestEqual(TEXT("property type"), Int->CppPropertyType, FString(TEXT("int32")));
			TestEqual(TEXT("default"), Int->CppDefaultValue, FString(TEXT("0")));

			FArticyPredefinedTypeBase* Bool = TypeInfo(TEXT("bool"));
			if (!TestNotNull(TEXT("bool entry"), Bool)) return;
			TestEqual(TEXT("cpp type"), Bool->CppType, FString(TEXT("bool")));
			TestEqual(TEXT("default"), Bool->CppDefaultValue, FString(TEXT("false")));

			FArticyPredefinedTypeBase* Float = TypeInfo(TEXT("float"));
			if (!TestNotNull(TEXT("float entry"), Float)) return;
			TestEqual(TEXT("cpp type"), Float->CppType, FString(TEXT("float")));
			TestEqual(TEXT("default"), Float->CppDefaultValue, FString(TEXT("0.f")));
		});

		It("maps uint onto int32 so it stays blueprint-exposable", [this, TypeInfo]()
		{
			FArticyPredefinedTypeBase* Uint = TypeInfo(TEXT("uint"));
			if (!TestNotNull(TEXT("uint entry"), Uint)) return;
			TestEqual(TEXT("cpp type"), Uint->CppType, FString(TEXT("int32")));
		});

		It("declares articy objects as pointers defaulting to nullptr", [this, TypeInfo]()
		{
			FArticyPredefinedTypeBase* Pin = TypeInfo(TEXT("InputPin"));
			if (!TestNotNull(TEXT("InputPin entry"), Pin)) return;
			TestEqual(TEXT("cpp type"), Pin->CppType, FString(TEXT("UArticyInputPin")));
			TestEqual(TEXT("property type"), Pin->CppPropertyType, FString(TEXT("UArticyInputPin*")));
			TestEqual(TEXT("default"), Pin->CppDefaultValue, FString(TEXT("nullptr")));
		});

		It("declares string and ftext with their own defaults", [this, TypeInfo]()
		{
			FArticyPredefinedTypeBase* String = TypeInfo(TEXT("string"));
			if (!TestNotNull(TEXT("string entry"), String)) return;
			TestEqual(TEXT("cpp type"), String->CppType, FString(TEXT("FString")));
			TestEqual(TEXT("default"), String->CppDefaultValue, FString(TEXT("TEXT(\"\")")));

			FArticyPredefinedTypeBase* Text = TypeInfo(TEXT("ftext"));
			if (!TestNotNull(TEXT("ftext entry"), Text)) return;
			TestEqual(TEXT("cpp type"), Text->CppType, FString(TEXT("FText")));
			TestEqual(TEXT("default"), Text->CppDefaultValue, FString(TEXT("FText::GetEmpty()")));
		});

		It("routes the localizable ArticyMultiLanguageString through FText", [this, TypeInfo]()
		{
			FArticyPredefinedTypeBase* MultiLanguage = TypeInfo(TEXT("ArticyMultiLanguageString"));
			if (!TestNotNull(TEXT("ArticyMultiLanguageString entry"), MultiLanguage)) return;
			TestEqual(TEXT("cpp type"), MultiLanguage->CppType, FString(TEXT("FText")));
			TestEqual(TEXT("default"), MultiLanguage->CppDefaultValue, FString(TEXT("FText::GetEmpty()")));
		});

		It("maps the non-localizable ArticyString onto FString", [this, TypeInfo]()
		{
			// Pinned because the mapping has been flipped to FText and back before.
			FArticyPredefinedTypeBase* ArticyString = TypeInfo(TEXT("ArticyString"));
			if (!TestNotNull(TEXT("ArticyString entry"), ArticyString)) return;
			TestEqual(TEXT("cpp type"), ArticyString->CppType, FString(TEXT("FString")));
			TestEqual(TEXT("default"), ArticyString->CppDefaultValue, FString(TEXT("TEXT(\"\")")));
		});

		It("keeps a placeholder item type for a generic array", [this, TypeInfo]()
		{
			// FArticyPropertyDef substitutes the real item type into this placeholder.
			FArticyPredefinedTypeBase* Array = TypeInfo(TEXT("array"));
			if (!TestNotNull(TEXT("array entry"), Array)) return;
			TestEqual(TEXT("cpp type"), Array->CppType, FString(TEXT("TArray<?>")));
		});

		It("exposes a shared generic enum descriptor as uint8", [this, TypeInfo]()
		{
			ArticyPredefinedTypeInfo<uint8>* EnumInfo = FArticyPredefTypes::GetEnum();
			if (!TestNotNull(TEXT("enum descriptor"), EnumInfo)) return;
			TestEqual(TEXT("cpp type"), EnumInfo->CppType, FString(TEXT("uint8")));
			// The named enum entries reuse that same descriptor.
			TestTrue(TEXT("LocationAnchorSize shares it"),
				TypeInfo(TEXT("LocationAnchorSize")) == static_cast<FArticyPredefinedTypeBase*>(EnumInfo));
		});
	});

	// Both kinds carry Unity markup to convert; only the localizable one is keyed for the string table.
	Describe("articy text deserializers", [this]()
	{
		const auto SetString = [](UArticyTestTextModel* Model, const TCHAR* ArticyType, const TCHAR* Property, const TCHAR* Value)
		{
			FArticyObjectDefinitions::SetProp(FName(ArticyType), FName(Property), Model, FString(TEXT("Obj.")) + Property,
				MakeShared<FJsonValueString>(Value), TEXT("Pkg"));
		};

		It("stores an ArticyString as the plain text", [this, SetString]()
		{
			UArticyTestTextModel* Model = NewObject<UArticyTestTextModel>();
			SetString(Model, TEXT("ArticyString"), TEXT("Plain"), TEXT("Hello"));
			TestEqual(TEXT("plain"), Model->Plain, FString(TEXT("Hello")));
		});

		It("stores an ArticyMultiLanguageString as an FText keyed by its path", [this, SetString]()
		{
			UArticyTestTextModel* Model = NewObject<UArticyTestTextModel>();
			SetString(Model, TEXT("ArticyMultiLanguageString"), TEXT("Localized"), TEXT("Obj.Localized"));

			// The export value is the string table key, which LocalizeString looks up at runtime.
			TestEqual(TEXT("value is the key"), Model->Localized.ToString(), FString(TEXT("Obj.Localized")));
			TestEqual(TEXT("namespace"), FTextInspector::GetNamespace(Model->Localized).Get(FString()), FString(TEXT("Pkg")));
			TestEqual(TEXT("key"), FTextInspector::GetKey(Model->Localized).Get(FString()), FString(TEXT("Obj.Localized")));
		});

		It("converts Unity markup in both kinds when the setting is on", [this, SetString]()
		{
			UArticyPluginSettings* Settings = GetMutableDefault<UArticyPluginSettings>();
			const bool bPrevious = Settings->bConvertUnityToUnrealRichText;
			Settings->bConvertUnityToUnrealRichText = true;

			UArticyTestTextModel* Model = NewObject<UArticyTestTextModel>();
			SetString(Model, TEXT("ArticyString"), TEXT("Plain"), TEXT("<b>Hi</b>"));
			SetString(Model, TEXT("ArticyMultiLanguageString"), TEXT("Localized"), TEXT("<i>Hi</i>"));

			Settings->bConvertUnityToUnrealRichText = bPrevious;

			TestEqual(TEXT("plain"), Model->Plain, FString(TEXT("<b>Hi</>")));
			TestEqual(TEXT("localized"), Model->Localized.ToString(), FString(TEXT("<i>Hi</>")));
		});

		It("keeps the markup when the setting is off", [this, SetString]()
		{
			UArticyPluginSettings* Settings = GetMutableDefault<UArticyPluginSettings>();
			const bool bPrevious = Settings->bConvertUnityToUnrealRichText;
			Settings->bConvertUnityToUnrealRichText = false;

			UArticyTestTextModel* Model = NewObject<UArticyTestTextModel>();
			SetString(Model, TEXT("ArticyString"), TEXT("Plain"), TEXT("<b>Hi</b>"));

			Settings->bConvertUnityToUnrealRichText = bPrevious;

			TestEqual(TEXT("plain"), Model->Plain, FString(TEXT("<b>Hi</b>")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
