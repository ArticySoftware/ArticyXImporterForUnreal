//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyTextExtension.h"
#include "ArticyTypeSystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTextExtensionSpec, "Articy.Runtime.TextExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	// Held across a $Type test so the singleton the resolver looks up cannot be collected
	// between registering a type and resolving against it.
	TStrongObjectPtr<UArticyTypeSystem> TypeSystem;
	TMap<FString, FArticyType> SavedTypes;
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

	// Token scanning without a world: every source lookup fails, so a token collapses to
	// its own name. That is the fallback authors see for a typo'd variable, and it is also
	// what keeps the scan loop from spinning on text it cannot resolve.
	Describe("Resolve: token scanning", [this]()
	{
		const auto ResolveText = [](const FString& In)
		{
			const FText Format = FText::FromString(In);
			return UArticyTextExtension::Get()->Resolve(nullptr, &Format).ToString();
		};

		It("replaces an unresolvable token with its own source name", [this, ResolveText]()
		{
			TestEqual(TEXT("token"), ResolveText(TEXT("[GameState.Flag]")), FString(TEXT("GameState.Flag")));
			TestEqual(TEXT("in a sentence"), ResolveText(TEXT("Flag is [GameState.Flag].")),
				FString(TEXT("Flag is GameState.Flag.")));
		});

		It("replaces every token in the text", [this, ResolveText]()
		{
			TestEqual(TEXT("two tokens"), ResolveText(TEXT("[A.B] and [C.D]")), FString(TEXT("A.B and C.D")));
		});

		It("leaves an unterminated token alone instead of looping", [this, ResolveText]()
		{
			TestEqual(TEXT("no closing bracket"), ResolveText(TEXT("50% off [Sale")), FString(TEXT("50% off [Sale")));
		});

		It("drops an empty token", [this, ResolveText]()
		{
			TestEqual(TEXT("empty"), ResolveText(TEXT("a[]b")), FString(TEXT("ab")));
		});

		It("ignores a closing bracket that comes before the opening one", [this, ResolveText]()
		{
			TestEqual(TEXT("stray bracket"), ResolveText(TEXT("a] [b.c]")), FString(TEXT("a] b.c")));
		});

		It("applies a :format suffix to the resolved value", [this, ResolveText]()
		{
			// A token may carry a C# style numeric format after a colon. The unresolved
			// source name is not a number, so it formats as zero - which is the point:
			// the format is applied to whatever the source returned.
			TestEqual(TEXT("zero pad"), ResolveText(TEXT("[A.B:000]")), FString(TEXT("000")));
		});

		It("resolves positional placeholders before tokens", [this]()
		{
			const FText Format = FText::FromString(TEXT("{0} sees [A.B]"));
			const FText Result = UArticyTextExtension::Get()->Resolve(nullptr, &Format, TEXT("Bob"));
			TestEqual(TEXT("both"), Result.ToString(), FString(TEXT("Bob sees A.B")));
		});
	});

	Describe("GetSource", [this]()
	{
		It("returns the source name when nothing can resolve it", [this]()
		{
			TestEqual(TEXT("gv"), UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("GameState.Flag")),
				FString(TEXT("GameState.Flag")));
		});

		It("returns the source name for an unknown $Type lookup", [this]()
		{
			TestEqual(TEXT("type"), UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("$Type.Nope.DisplayName")),
				FString(TEXT("$Type.Nope.DisplayName")));
		});

		It("returns an empty string for an empty source name", [this]()
		{
			TestEqual(TEXT("empty"), UArticyTextExtension::Get()->Test_GetSource(nullptr, FString()), FString());
		});
	});

	// [$Type.<TypeName>.<Property>] resolves to the property's declared type through the
	// type system. Unit tests run in a host project with no imported articy content, so the
	// type system singleton is the empty placeholder and the spec can register types on it.
	Describe("$Type tokens", [this]()
	{
		// Regression: GetSource tested the already-split first part against the prefix
		// "$Type.", which can never match, so every $Type token fell through unresolved.
		BeforeEach([this]()
		{
			TypeSystem.Reset(UArticyTypeSystem::Get());
			if (TypeSystem.IsValid())
			{
				SavedTypes = TypeSystem->Types;

				FArticyPropertyInfo DisplayName;
				DisplayName.TechnicalName = TEXT("DisplayName");
				DisplayName.PropertyType = TEXT("string");

				FArticyPropertyInfo Motivation;
				Motivation.TechnicalName = TEXT("Character.Motivation");
				Motivation.PropertyType = TEXT("int");
				Motivation.IsTemplateProperty = true;

				FArticyType Character;
				Character.TechnicalName = TEXT("Character");
				Character.Properties = { DisplayName, Motivation };

				TypeSystem->Types.Add(TEXT("Character"), Character);
			}
		});

		AfterEach([this]()
		{
			if (TypeSystem.IsValid())
			{
				TypeSystem->Types = SavedTypes;
			}
			TypeSystem.Reset();
			SavedTypes.Reset();
		});

		It("resolves a type property to its declared type", [this]()
		{
			const FText Format = FText::FromString(TEXT("[$Type.Character.DisplayName]"));
			TestEqual(TEXT("resolved"), UArticyTextExtension::Get()->Resolve(nullptr, &Format).ToString(),
				FString(TEXT("string")));
		});

		It("resolves a feature property by its qualified name", [this]()
		{
			const FText Format = FText::FromString(TEXT("[$Type.Character.Character.Motivation]"));
			TestEqual(TEXT("resolved"), UArticyTextExtension::Get()->Resolve(nullptr, &Format).ToString(),
				FString(TEXT("int")));
		});

		It("resolves a $Type token without a world context", [this]()
		{
			// Type metadata is world-independent, unlike variables and objects.
			TestEqual(TEXT("no world"), UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("$Type.Character.DisplayName")),
				FString(TEXT("string")));
		});

		It("falls back to the source name for an unknown property", [this]()
		{
			TestEqual(TEXT("unknown property"),
				UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("$Type.Character.Nope")),
				FString(TEXT("$Type.Character.Nope")));
		});

		It("falls back to the source name when no property is named", [this]()
		{
			TestEqual(TEXT("no property"), UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("$Type.Character")),
				FString(TEXT("$Type.Character")));
			TestEqual(TEXT("marker only"), UArticyTextExtension::Get()->Test_GetSource(nullptr, TEXT("$Type")),
				FString(TEXT("$Type")));
		});
	});

	// Method tokens are written as [<anything>.method(arg,arg,...)]: the part before the
	// dot is ignored, the args are split on commas.
	Describe("method tokens", [this]()
	{
		It("dispatches to a registered user method with its arguments", [this]()
		{
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			Ext->Test_ClearUserMethods();
			Ext->AddUserMethod(TEXT("join"), [](const TArray<FString>& Args)
			{
				return FString::Join(Args, TEXT("|"));
			});

			TestEqual(TEXT("args passed through"), Ext->Test_GetSource(nullptr, TEXT("x.join(a,b,c)")),
				FString(TEXT("a|b|c")));
			Ext->Test_ClearUserMethods();
		});

		It("returns an empty string for an unregistered method", [this]()
		{
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			Ext->Test_ClearUserMethods();
			TestEqual(TEXT("unknown"), Ext->Test_GetSource(nullptr, TEXT("x.nosuchmethod(a)")), FString());
		});

		It("picks a branch with the built-in if method", [this]()
		{
			// if(<format>, <substituted into {0}>, <then>, <else>) - the format is resolved
			// and compared against "1".
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			TestEqual(TEXT("then"), Ext->Test_GetSource(nullptr, TEXT("x.if({0},1,yes,no)")), FString(TEXT("yes")));
			TestEqual(TEXT("else"), Ext->Test_GetSource(nullptr, TEXT("x.if({0},0,yes,no)")), FString(TEXT("no")));
		});

		It("inverts the branch with the built-in not method", [this]()
		{
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			TestEqual(TEXT("inverted then"), Ext->Test_GetSource(nullptr, TEXT("x.not({0},1,yes,no)")), FString(TEXT("no")));
			TestEqual(TEXT("inverted else"), Ext->Test_GetSource(nullptr, TEXT("x.not({0},0,yes,no)")), FString(TEXT("yes")));
		});

		It("treats a missing else branch as an empty string", [this]()
		{
			// Regression: the else branch used to be read unconditionally, so a
			// three-argument if indexed past the end of the argument array.
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			const TArray<FString> IfNoElse{ TEXT("{0}"), TEXT("0"), TEXT("yes") };
			const TArray<FString> NotNoElse{ TEXT("{0}"), TEXT("1"), TEXT("yes") };
			TestEqual(TEXT("if without else"), Ext->Test_ExecuteMethod(nullptr, TEXT("if"), IfNoElse), FString());
			TestEqual(TEXT("not without else"), Ext->Test_ExecuteMethod(nullptr, TEXT("not"), NotNoElse), FString());
		});

		It("returns an empty string when a built-in gets too few arguments", [this]()
		{
			UArticyTextExtension* Ext = UArticyTextExtension::Get();
			const TArray<FString> TooFew{ TEXT("{0}"), TEXT("1") };
			TestEqual(TEXT("if with two args"), Ext->Test_ExecuteMethod(nullptr, TEXT("if"), TooFew), FString());
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

		It("keeps the sign in front of the zero padding", [this, Format]()
		{
			TestEqual(TEXT("negative"), Format(TEXT("-42"), TEXT("00000")), FString(TEXT("-00042")));
		});

		It("does not truncate a value beyond 32-bit range", [this, Format]()
		{
			// Regression: the padded form used to hand a 32-bit value to a 64-bit printf
			// specifier, which produced garbage rather than the number.
			TestEqual(TEXT("large"), Format(TEXT("5000000000"), TEXT("0")), FString(TEXT("5000000000")));
		});

		It("returns a bare 0 for a non-numeric source value", [this, Format]()
		{
			TestEqual(TEXT("not a number"), Format(TEXT("GameState.Flag"), TEXT("0")), FString(TEXT("0")));
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
