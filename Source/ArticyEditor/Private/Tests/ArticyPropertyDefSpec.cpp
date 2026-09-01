//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "CodeGeneration/CodeFileGenerator.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyPropertyDefSpec, "Articy.Editor.ObjectDefinitions.PropertyDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPropertyDefSpec)

void FArticyPropertyDefSpec::Define()
{
	Describe("ImportFromJson", [this]()
	{
		It("parses a non-localized property and keeps its type", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("CustomStr"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("property name"), Def.GetPropetyName().ToString(), FString(TEXT("CustomStr")));
			TestEqual(TEXT("type kept"), Def.GetOriginalType().ToString(), FString(TEXT("string")));
		});

		It("promotes a localized string property (e.g. Text) to FText", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("Text"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("promoted to FText"), Def.GetOriginalType().ToString(), FString(TEXT("FText")));
		});
	});

	// The two articy text kinds must end up as different C++ types with different getters.
	Describe("articy text kinds", [this]()
	{
		const auto MakeDef = [](const TCHAR* Property, const TCHAR* Type, UArticyImportData* Data)
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), Property);
			Json->SetStringField(TEXT("Type"), Type);

			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);
			return Def;
		};

		It("maps ArticyString onto FString and ArticyMultiLanguageString onto FText", [this, MakeDef]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			TestEqual(TEXT("plain"), MakeDef(TEXT("VoiceActor"), TEXT("ArticyString"), Data).GetCppType(Data), FString(TEXT("FString")));
			TestEqual(TEXT("localized"), MakeDef(TEXT("Motivation"), TEXT("ArticyMultiLanguageString"), Data).GetCppType(Data), FString(TEXT("FText")));
		});

		It("tells the two kinds apart", [this, MakeDef]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();

			const FArticyPropertyDef Plain = MakeDef(TEXT("VoiceActor"), TEXT("ArticyString"), Data);
			TestTrue(TEXT("plain is plain"), Plain.IsPlainText());
			TestFalse(TEXT("plain is not localized"), Plain.IsLocalizedText());

			const FArticyPropertyDef Localized = MakeDef(TEXT("Motivation"), TEXT("ArticyMultiLanguageString"), Data);
			TestTrue(TEXT("localized is localized"), Localized.IsLocalizedText());
			TestFalse(TEXT("localized is not plain"), Localized.IsPlainText());

			// A legacy export's promoted string counts as localized too.
			TestTrue(TEXT("promoted legacy string"), MakeDef(TEXT("Text"), TEXT("string"), Data).IsLocalizedText());

			const FArticyPropertyDef Number = MakeDef(TEXT("HP"), TEXT("int"), Data);
			TestFalse(TEXT("int is not plain"), Number.IsPlainText());
			TestFalse(TEXT("int is not localized"), Number.IsLocalizedText());
		});

		It("generates a localized getter for a multi-language string and a resolved one for a plain string", [this, MakeDef]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			const FArticyPropertyDef Localized = MakeDef(TEXT("Motivation"), TEXT("ArticyMultiLanguageString"), Data);
			const FArticyPropertyDef Plain = MakeDef(TEXT("VoiceActor"), TEXT("ArticyString"), Data);
			const FArticyPropertyDef Reserved = MakeDef(TEXT("DisplayName"), TEXT("ArticyString"), Data);

			// The generator only writes into the project's ArticyGenerated folder.
			const FString FileName = TEXT("ArticyUnitTestTextAccessors.h");
			CodeFileGenerator(FileName, true, [&](CodeFileGenerator* Header)
			{
				Localized.GenerateCode(*Header, Data);
				Plain.GenerateCode(*Header, Data);
				Reserved.GenerateCode(*Header, Data);
			});

			const FString Path = CodeGenerator::GetSourceFolder() / FileName;
			FString Content;
			TestTrue(TEXT("file written"), FFileHelper::LoadFileToString(Content, *Path));
			IFileManager::Get().Delete(*Path);

			TestTrue(TEXT("localized property"), Content.Contains(TEXT("FText Motivation = FText::GetEmpty();")));
			TestTrue(TEXT("localized getter"), Content.Contains(TEXT("FText GetMotivation() { return GetPropertyText(Motivation); }")));
			TestTrue(TEXT("plain property"), Content.Contains(TEXT("FString VoiceActor = TEXT(\"\");")));
			TestTrue(TEXT("resolved getter"), Content.Contains(TEXT("FText GetVoiceActor() { return ResolvePropertyString(VoiceActor); }")));
			// DisplayName is served by IArticyObjectWithDisplayName::GetDisplayName instead.
			TestFalse(TEXT("no getter for a reserved name"), Content.Contains(TEXT("GetDisplayName()")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
