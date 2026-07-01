//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyTexts.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTextsSpec, "Articy.Editor.Texts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTextsSpec)

void FArticyTextsSpec::Define()
{
	Describe("ImportFromJson", [this]()
	{
		It("reads the context and each text definition", [this]()
		{
			TSharedPtr<FJsonObject> Greeting = MakeShared<FJsonObject>();
			Greeting->SetStringField(TEXT("Text"), TEXT("Hello"));
			Greeting->SetStringField(TEXT("VoAsset"), TEXT("VO_Hello"));

			TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetStringField(TEXT("Context"), TEXT("MyContext"));
			Root->SetObjectField(TEXT("Greeting"), Greeting);

			FArticyTexts Texts;
			Texts.ImportFromJson(Root);

			TestEqual(TEXT("context"), Texts.Context, FString(TEXT("MyContext")));
			TestTrue(TEXT("has entry"), Texts.Content.Contains(TEXT("Greeting")));
			if (const FArticyTextDef* Def = Texts.Content.Find(TEXT("Greeting")))
			{
				TestEqual(TEXT("text"), Def->Text, FString(TEXT("Hello")));
				TestEqual(TEXT("vo asset"), Def->VoAsset, FString(TEXT("VO_Hello")));
			}
		});

		It("does nothing for an invalid json object", [this]()
		{
			FArticyTexts Texts;
			Texts.ImportFromJson(nullptr);
			TestEqual(TEXT("no entries"), Texts.Content.Num(), 0);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
