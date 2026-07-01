//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyTextExtension.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTextExtensionSpec, "Articy.Runtime.TextExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTextExtensionSpec)

void FArticyTextExtensionSpec::Define()
{
	Describe("Resolve", [this]()
	{
		It("returns empty text for a null format", [this]()
		{
			TestTrue(TEXT("null format"), UArticyTextExtension::Get()->Resolve(nullptr, nullptr, TEXT("x")).IsEmpty());
		});

		It("leaves text without placeholders unchanged", [this]()
		{
			const FText Format = FText::FromString(TEXT("No placeholders here"));
			const FText Result = UArticyTextExtension::Get()->Resolve(nullptr, &Format);
			TestEqual(TEXT("unchanged"), Result.ToString(), FString(TEXT("No placeholders here")));
		});

		It("replaces a positional {0} placeholder", [this]()
		{
			const FText Format = FText::FromString(TEXT("Hello {0}"));
			const FText Result = UArticyTextExtension::Get()->Resolve(nullptr, &Format, TEXT("World"));
			TestEqual(TEXT("single arg"), Result.ToString(), FString(TEXT("Hello World")));
		});

		It("replaces multiple positional placeholders", [this]()
		{
			const FText Format = FText::FromString(TEXT("{0} and {1}"));
			const FText Result = UArticyTextExtension::Get()->Resolve(nullptr, &Format, TEXT("A"), TEXT("B"));
			TestEqual(TEXT("two args"), Result.ToString(), FString(TEXT("A and B")));
		});
	});

	Describe("FormatNumber", [this]()
	{
		const auto Format = [](const FString& Value, const FString& Fmt)
		{
			return UArticyTextExtension::Get()->Test_FormatNumber(Value, Fmt);
		};

		It("rounds to a whole number for the '0' format", [this, Format]()
		{
			TestEqual(TEXT("exact"), Format(TEXT("5"), TEXT("0")), FString(TEXT("5")));
			TestEqual(TEXT("rounds up"), Format(TEXT("5.7"), TEXT("0")), FString(TEXT("6")));
		});

		It("zero-pads to the number of '0' digits", [this, Format]()
		{
			TestEqual(TEXT("pad"), Format(TEXT("42"), TEXT("000")), FString(TEXT("042")));
		});

		It("keeps the given number of '#' decimals", [this, Format]()
		{
			TestEqual(TEXT("two decimals"), Format(TEXT("3.14159"), TEXT("##")), FString(TEXT("3.14")));
		});

		It("treats true/false as 1/0", [this, Format]()
		{
			TestEqual(TEXT("true"), Format(TEXT("true"), TEXT("0")), FString(TEXT("1")));
			TestEqual(TEXT("false"), Format(TEXT("false"), TEXT("0")), FString(TEXT("0")));
		});

		It("passes through literal characters in the format", [this, Format]()
		{
			TestEqual(TEXT("suffix"), Format(TEXT("5"), TEXT("0 pts")), FString(TEXT("5 pts")));
		});
	});

	Describe("ResolveBoolean", [this]()
	{
		It("falls back to \"true\"/\"false\" when no localizer is present", [this]()
		{
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			TestEqual(TEXT("true"), Ext->Test_ResolveBoolean(nullptr, TEXT("SomeVar"), true), FString(TEXT("true")));
			TestEqual(TEXT("false"), Ext->Test_ResolveBoolean(nullptr, TEXT("SomeVar"), false), FString(TEXT("false")));
		});
	});

	Describe("SplitInstance", [this]()
	{
		It("splits a name and its <instance> number", [this]()
		{
			FString Name, Instance;
			UArticyTextExtension::Test_SplitInstance(TEXT("Hero<3>"), Name, Instance);
			TestEqual(TEXT("name"), Name, FString(TEXT("Hero")));
			TestEqual(TEXT("instance"), Instance, FString(TEXT("3")));
		});

		It("defaults the instance to 0 when no <...> is present", [this]()
		{
			FString Name, Instance;
			UArticyTextExtension::Test_SplitInstance(TEXT("Hero"), Name, Instance);
			TestEqual(TEXT("name"), Name, FString(TEXT("Hero")));
			TestEqual(TEXT("instance"), Instance, FString(TEXT("0")));
		});

		It("defaults the instance to 0 when the closing '>' is missing", [this]()
		{
			FString Name, Instance;
			UArticyTextExtension::Test_SplitInstance(TEXT("Hero<3"), Name, Instance);
			TestEqual(TEXT("name"), Name, FString(TEXT("Hero<3")));
			TestEqual(TEXT("instance"), Instance, FString(TEXT("0")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
