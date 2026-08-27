//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyHelpers.h"
#include "ArticyTestTextObject.h"

#if WITH_AUTOMATION_TESTS

// The text accessors used to assume an FText and reinterpreted an FString property; they now
// go by the reflected type, so both kinds are pinned here.
BEGIN_DEFINE_SPEC(FArticyTextPropertiesSpec, "Articy.Runtime.TextProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTextPropertiesSpec)

void FArticyTextPropertiesSpec::Define()
{
	Describe("ArticyHelpers text property helpers", [this]()
	{
		It("classifies FString and FText properties and nothing else", [this]()
		{
			const UClass* Class = UArticyTestTextObject::StaticClass();
			TestTrue(TEXT("FString"), ArticyHelpers::GetTextPropertyKind(Class->FindPropertyByName(TEXT("DisplayName")))
				== ArticyHelpers::EArticyTextPropertyKind::PlainString);
			TestTrue(TEXT("FText"), ArticyHelpers::GetTextPropertyKind(Class->FindPropertyByName(TEXT("Text")))
				== ArticyHelpers::EArticyTextPropertyKind::LocalizedText);
			TestTrue(TEXT("other"), ArticyHelpers::GetTextPropertyKind(Class->FindPropertyByName(TEXT("Number")))
				== ArticyHelpers::EArticyTextPropertyKind::None);
			TestTrue(TEXT("null"), ArticyHelpers::GetTextPropertyKind(nullptr) == ArticyHelpers::EArticyTextPropertyKind::None);
		});

		It("reads the raw value of both kinds", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->DisplayName = TEXT("Questions");
			Object->Text = FText::FromString(TEXT("Hub_1.Text"));
			const UClass* Class = Object->GetClass();

			TestEqual(TEXT("plain"), ArticyHelpers::GetTextPropertyKey(Object, Class->FindPropertyByName(TEXT("DisplayName"))),
				FString(TEXT("Questions")));
			TestEqual(TEXT("key"), ArticyHelpers::GetTextPropertyKey(Object, Class->FindPropertyByName(TEXT("Text"))),
				FString(TEXT("Hub_1.Text")));
			TestTrue(TEXT("other"), ArticyHelpers::GetTextPropertyKey(Object, Class->FindPropertyByName(TEXT("Number"))).IsEmpty());
		});

		It("writes both kinds from an FText", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			const UClass* Class = Object->GetClass();

			TestTrue(TEXT("plain written"), ArticyHelpers::SetTextPropertyValue(Object, Class->FindPropertyByName(TEXT("DisplayName")),
				FText::FromString(TEXT("Answers"))));
			TestTrue(TEXT("text written"), ArticyHelpers::SetTextPropertyValue(Object, Class->FindPropertyByName(TEXT("Text")),
				FText::FromString(TEXT("Hub_2.Text"))));
			TestFalse(TEXT("other refused"), ArticyHelpers::SetTextPropertyValue(Object, Class->FindPropertyByName(TEXT("Number")),
				FText::FromString(TEXT("1"))));

			TestEqual(TEXT("plain"), Object->DisplayName, FString(TEXT("Answers")));
			TestEqual(TEXT("text"), Object->Text.ToString(), FString(TEXT("Hub_2.Text")));
		});

		It("uses the backup text for an empty plain string", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			const FText Backup = FText::FromString(TEXT("..."));
			const FText Value = ArticyHelpers::GetTextPropertyValue(Object, Object->GetClass()->FindPropertyByName(TEXT("DisplayName")), true, &Backup);
			TestEqual(TEXT("backup"), Value.ToString(), FString(TEXT("...")));
		});

		It("returns empty text for a missing property", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			TestTrue(TEXT("empty"), ArticyHelpers::GetTextPropertyValue(Object, nullptr).IsEmpty());
		});
	});

	Describe("plain string properties (ArticyString)", [this]()
	{
		It("returns a display name stored as FString", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->DisplayName = TEXT("Questions");

			IArticyObjectWithDisplayName* Named = Object;
			TestEqual(TEXT("display name"), Named->GetDisplayName().ToString(), FString(TEXT("Questions")));
		});

		It("resolves text extension tokens in the value", [this]()
		{
			// Without a world no token can resolve, so it collapses to its own source name.
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->DisplayName = TEXT("Ask [GameState.Name]");

			IArticyObjectWithDisplayName* Named = Object;
			TestEqual(TEXT("resolved"), Named->GetDisplayName().ToString(), FString(TEXT("Ask GameState.Name")));
		});

		It("writes a display name through the setter", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->DisplayName = TEXT("Questions");

			IArticyObjectWithDisplayName* Named = Object;
			const FText Stored = Named->SetDisplayName(FText::FromString(TEXT("Answers")));
			TestEqual(TEXT("returned"), Stored.ToString(), FString(TEXT("Answers")));
			TestEqual(TEXT("property"), Object->DisplayName, FString(TEXT("Answers")));
		});

		It("returns stage directions stored as FString", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->StageDirections = TEXT("(whispers)");

			IArticyObjectWithStageDirections* Directed = Object;
			TestEqual(TEXT("stage directions"), Directed->GetStageDirections().ToString(), FString(TEXT("(whispers)")));
		});

		It("does not look up a VO asset for a plain text", [this]()
		{
			// VO assets are exported with the localized texts, so there is no key to look up.
			UArticyTestPlainTextObject* Object = NewObject<UArticyTestPlainTextObject>();
			Object->Text = TEXT("Hello");

			IArticyObjectWithText* WithText = Object;
			TestEqual(TEXT("text"), WithText->GetText().ToString(), FString(TEXT("Hello")));
			TestNull(TEXT("no VO asset"), WithText->GetVOAsset(nullptr));
		});
	});

	Describe("localized text properties (ArticyMultiLanguageString)", [this]()
	{
		It("looks the key up through the localizer", [this]()
		{
			// The unit test host has no string table, so the lookup falls back to the key.
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();
			Object->Text = FText::FromString(TEXT("Articy.Tests.Obj.Text"));

			IArticyObjectWithText* WithText = Object;
			TestEqual(TEXT("key"), WithText->GetText().ToString(), FString(TEXT("Articy.Tests.Obj.Text")));
		});

		It("falls back to the backup text for an empty menu text", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();

			IArticyObjectWithMenuText* WithMenu = Object;
			TestEqual(TEXT("backup"), WithMenu->GetMenuText().ToString(), FString(TEXT("...")));
		});

		It("writes text through the setter", [this]()
		{
			UArticyTestTextObject* Object = NewObject<UArticyTestTextObject>();

			IArticyObjectWithText* WithText = Object;
			const FText Stored = WithText->SetText(FText::FromString(TEXT("Articy.Tests.Obj.Text2")));
			TestEqual(TEXT("returned"), Stored.ToString(), FString(TEXT("Articy.Tests.Obj.Text2")));
			TestEqual(TEXT("property"), Object->Text.ToString(), FString(TEXT("Articy.Tests.Obj.Text2")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
