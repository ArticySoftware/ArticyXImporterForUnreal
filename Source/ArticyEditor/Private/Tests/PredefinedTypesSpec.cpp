//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "PredefinedTypes.h"

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
}

#endif // WITH_AUTOMATION_TESTS
