//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyTextExtension.h"
#include "ArticyTextExtensionTestFixtures.h"
#include "ArticyTestLocalizer.h"
#include "ArticyPackage.h"
#include "ArticyPluginSettings.h"
#include "ArticyLocalizerSystem.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// Object ids of the fixture content. Hex in tokens, decimal in object representations.
	constexpr uint64 ManfredId = 0x0100000000000001ull;
	constexpr uint64 HamsterId = 0x0100000000000002ull;
	constexpr uint64 SwordId = 0x0100000000000003ull;
	constexpr uint64 ShieldId = 0x0100000000000004ull;
	constexpr uint64 FragmentId = 0x0100000000000005ull;

	const TCHAR* PackageName = TEXT("TextExtensionTestPackage");
	const FName StringTableId = TEXT("ARTICY");

	FString Representation(uint64 Id, int32 CloneId = 0)
	{
		return FString::Printf(TEXT("%llu_%d"), Id, CloneId);
	}

	FArticyEnumValueInfo MakeEnumValue(const TCHAR* Name, int Value)
	{
		FArticyEnumValueInfo Info;
		Info.LocaKey_DisplayName = Name;
		Info.TechnicalName = Name;
		Info.Value = Value;
		return Info;
	}

	FArticyPropertyInfo MakePropertyInfo(const TCHAR* Name, const TCHAR* Type)
	{
		FArticyPropertyInfo Info;
		Info.LocaKey_DisplayName = Name;
		Info.PropertyType = Type;
		return Info;
	}
}

// Every scenario runs against an in-memory database (objects, template features, reference
// slots and strips, script fragments, an enum), an in-memory global variable set and a
// hand-built type system, injected through UArticyTestTextExtension. Nothing here needs
// imported content; the integration spec covers the generated classes.
BEGIN_DEFINE_SPEC(FArticyTextExtensionSpec, "Articy.Runtime.TextExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UArticyTestTextExtension* Ext = nullptr;
	UArticyDatabase* DB = nullptr;
	UArticyTextTestGlobalVariables* GVs = nullptr;
	UArticyTypeSystem* TypeSystem = nullptr;
	UArticyTextTestEntity* Manfred = nullptr;  // the loaded (database-owned) clone, usable as Outer
	UArticyTextTestEntity* Fragment = nullptr; // a dialogue fragment spoken by Manfred
	TArray<UObject*> Rooted;

	FString Resolve(const FString& Format, UObject* Outer = nullptr) const
	{
		return Ext->Resolve(Outer, Format).ToString();
	}

	UArticyTextTestEntity* MakeEntity(UObject* Package, uint64 Id, const TCHAR* TechnicalName, const TCHAR* DisplayName)
	{
		UArticyTextTestEntity* Entity = NewObject<UArticyTextTestEntity>(Package);
		Entity->SetIdentity(Id, TechnicalName);
		Entity->DisplayName = FText::FromString(DisplayName);
		Entity->ArticyType.TechnicalName = TEXT("Character");
		Entity->ArticyType.LocaKey_DisplayName = TEXT("Character");
		return Entity;
	}

	UArticyTextTestFeature* MakeFeature(UArticyTextTestEntity* Owner, int32 HP, const TCHAR* ConditionExpression)
	{
		UArticyTextTestFeature* Feature = NewObject<UArticyTextTestFeature>(Owner);
		Feature->HP = HP;
		Feature->IsStrong = NewObject<UArticyTextTestCondition>(Feature);
		Feature->IsStrong->SetExpression(ConditionExpression);
		Feature->Heal = NewObject<UArticyTextTestInstruction>(Feature);
		Feature->Heal->SetExpression(TEXT("heal"));
		Owner->Character = Feature;
		return Feature;
	}

	void Root(UObject* Object)
	{
		Object->AddToRoot();
		Rooted.Add(Object);
	}

	// Localization scaffolding: an in-memory ARTICY string table plus the test localizer in its "loaded" state.
	UArticyTestLocalizer* Localizer = nullptr;
	bool bStringTableRegistered = false;

	bool BeginLocalization(const TMap<FString, FString>& Entries)
	{
		Localizer = Cast<UArticyTestLocalizer>(UArticyLocalizerSystem::Get());
		if (!Localizer)
		{
			AddWarning(TEXT("No UArticyTestLocalizer is active (another localizer subclass is loaded); skipping."));
			return false;
		}
		FStringTableRegistry::Get().UnregisterStringTable(StringTableId);
		FStringTableRef Table = FStringTable::NewStringTable();
		Table->SetNamespace(StringTableId.ToString());
		for (const auto& Entry : Entries)
		{
			Table->SetSourceString(FTextKey(Entry.Key), Entry.Value);
		}
		FStringTableRegistry::Get().RegisterStringTable(StringTableId, Table);
		bStringTableRegistered = true;
		Localizer->SetDataLoaded(true);
		return true;
	}

	void EndLocalization()
	{
		if (bStringTableRegistered)
		{
			FStringTableRegistry::Get().UnregisterStringTable(StringTableId);
			bStringTableRegistered = false;
		}
		if (Localizer)
		{
			Localizer->SetDataLoaded(false);
			Localizer = nullptr;
		}
	}
END_DEFINE_SPEC(FArticyTextExtensionSpec)

void FArticyTextExtensionSpec::Define()
{
	BeforeEach([this]()
	{
		// --- database with a package of objects
		DB = NewObject<UArticyDatabase>();
		Root(DB);
		DB->SetExpressoScriptsClass(UArticyTextTestExpresso::StaticClass());

		UArticyPackage* Package = NewObject<UArticyPackage>(DB);
		Package->Name = PackageName;

		UArticyTextTestEntity* ManfredAsset = MakeEntity(Package, ManfredId, TEXT("Chr_Manfred"), TEXT("Manfred"));
		ManfredAsset->ZIndex = 4;
		ManfredAsset->Nickname = TEXT("Manny");
		UArticyTextTestFeature* Character = MakeFeature(ManfredAsset, 42, TEXT("strong"));
		Character->HitRating = 0.42f;
		Character->IsHero = true;
		Character->Motivation = TEXT("Revenge");
		Character->Title = FText::FromString(TEXT("Loca.Title"));
		Character->Companion = FArticyId{ HamsterId };
		Character->Inventory = { FArticyId{ SwordId }, FArticyId{ ShieldId } };
		Character->ClassEnum = EArticyTextTestClass::Bard;
		Character->Partner = Representation(HamsterId);

		UArticyTextTestEntity* HamsterAsset = MakeEntity(Package, HamsterId, TEXT("Chr_Hamster"), TEXT("Hamster"));
		MakeFeature(HamsterAsset, 7, TEXT("weak"));

		UArticyTextTestEntity* SwordAsset = MakeEntity(Package, SwordId, TEXT("Itm_Sword"), TEXT("Sword"));
		UArticyTextTestEntity* ShieldAsset = MakeEntity(Package, ShieldId, TEXT("Itm_Shield"), TEXT("Shield"));

		UArticyTextTestEntity* FragmentAsset = MakeEntity(Package, FragmentId, TEXT("DFr_Hello"), TEXT(""));
		FragmentAsset->Speaker = FArticyId{ ManfredId };

		for (UArticyObject* Asset : { ManfredAsset, HamsterAsset, SwordAsset, ShieldAsset, FragmentAsset })
		{
			Package->AddAsset(Asset);
		}
		DB->SetLoadedPackages({ Package });
		DB->LoadPackage(PackageName);

		Manfred = Cast<UArticyTextTestEntity>(DB->GetObjectByName(TEXT("Chr_Manfred")));
		Fragment = Cast<UArticyTextTestEntity>(DB->GetObjectByName(TEXT("DFr_Hello")));

		// --- global variables
		GVs = NewObject<UArticyTextTestGlobalVariables>();
		Root(GVs);
		GVs->Populate(Representation(HamsterId));

		// --- type system
		TypeSystem = NewObject<UArticyTypeSystem>();
		Root(TypeSystem);
		FArticyType CharacterType;
		CharacterType.TechnicalName = TEXT("Character");
		CharacterType.LocaKey_DisplayName = TEXT("Character");
		CharacterType.HasTemplate = true;
		// generated classes are named U<Project><Type>; the type system records that name
		CharacterType.CPPType = TEXT("U") + UArticyTextTestEntity::StaticClass()->GetName();
		CharacterType.Properties = {
			MakePropertyInfo(TEXT("DisplayName"), TEXT("string")),
			MakePropertyInfo(TEXT("HP"), TEXT("int")),
			MakePropertyInfo(TEXT("Attributes.Strength"), TEXT("int")) };
		TypeSystem->Types.Add(TEXT("Character"), CharacterType);
		FArticyType ClassType;
		ClassType.TechnicalName = TEXT("CharacterClass");
		ClassType.LocaKey_DisplayName = TEXT("CharacterClass");
		ClassType.IsEnum = true;
		ClassType.CPPType = TEXT("EArticyTextTestClass");
		ClassType.EnumValues = { MakeEnumValue(TEXT("Warrior"), 0), MakeEnumValue(TEXT("Mage"), 1), MakeEnumValue(TEXT("Bard"), 2) };
		TypeSystem->Types.Add(TEXT("CharacterClass"), ClassType);

		// --- the extension under test
		Ext = NewObject<UArticyTestTextExtension>();
		Root(Ext);
		Ext->Database = DB;
		Ext->GlobalVariables = GVs;
		Ext->TypeSystem = TypeSystem;
	});

	AfterEach([this]()
	{
		EndLocalization();
		for (UObject* Object : Rooted)
		{
			Object->RemoveFromRoot();
		}
		Rooted.Reset();
		Ext = nullptr;
		DB = nullptr;
		GVs = nullptr;
		TypeSystem = nullptr;
		Manfred = nullptr;
		Fragment = nullptr;
	});

	Describe("fixture", [this]()
	{
		It("loads the objects into the database", [this]()
		{
			TestNotNull(TEXT("Manfred"), Manfred);
			TestNotNull(TEXT("Fragment"), Fragment);
			TestNotNull(TEXT("by id"), DB->GetObject(FArticyId{ HamsterId }));
		});
	});

	Describe("Resolve", [this]()
	{
		It("returns empty text for a null format", [this]()
		{
			TestTrue(TEXT("null format"), Ext->Resolve(nullptr, nullptr, TEXT("x")).IsEmpty());
		});

		It("leaves text without placeholders unchanged", [this]()
		{
			const FText Format = FText::FromString(TEXT("No placeholders here"));
			TestEqual(TEXT("unchanged"), Ext->Resolve(nullptr, &Format).ToString(), FString(TEXT("No placeholders here")));
		});

		It("replaces a positional {0} placeholder", [this]()
		{
			const FText Format = FText::FromString(TEXT("Hello {0}"));
			TestEqual(TEXT("single arg"), Ext->Resolve(nullptr, &Format, TEXT("World")).ToString(), FString(TEXT("Hello World")));
		});

		It("replaces multiple positional placeholders", [this]()
		{
			const FText Format = FText::FromString(TEXT("{0} and {1}"));
			TestEqual(TEXT("two args"), Ext->Resolve(nullptr, &Format, TEXT("A"), TEXT("B")).ToString(), FString(TEXT("A and B")));
		});

		It("accepts numbers, strings and texts as positional parameters", [this]()
		{
			TestEqual(TEXT("int"), Ext->Resolve(nullptr, FString(TEXT("Need {0}g")), 24).ToString(), FString(TEXT("Need 24g")));
			TestEqual(TEXT("float"), Ext->Resolve(nullptr, FString(TEXT("{0}")), 1.5).ToString(), FString(TEXT("1.5")));
			TestEqual(TEXT("bool"), Ext->Resolve(nullptr, FString(TEXT("{0}")), true).ToString(), FString(TEXT("true")));
			TestEqual(TEXT("FString"), Ext->Resolve(nullptr, FString(TEXT("{0}")), FString(TEXT("s"))).ToString(), FString(TEXT("s")));
			TestEqual(TEXT("FText"), Ext->Resolve(nullptr, FString(TEXT("{0}")), FText::FromString(TEXT("t"))).ToString(), FString(TEXT("t")));
		});

		It("inserts a positional parameter into a token before resolving it", [this]()
		{
			// the Unity docs example: Session.Player{0} with i + 1
			TestEqual(TEXT("player 1"), Ext->Resolve(nullptr, FString(TEXT("Hello, [Session.Player{0}]")), 1).ToString(), FString(TEXT("Hello, Bob")));
			TestEqual(TEXT("player 2"), Ext->Resolve(nullptr, FString(TEXT("Hello, [Session.Player{0}]")), 2).ToString(), FString(TEXT("Hello, Anna")));
		});

		It("resolves positional placeholders before tokens", [this]()
		{
			TestEqual(TEXT("both"), Ext->Resolve(nullptr, FString(TEXT("{0} sees [Chr_Hamster]")), TEXT("Bob")).ToString(),
				FString(TEXT("Bob sees Hamster")));
		});
	});

	Describe("escape sequences", [this]()
	{
		It("turns escaped brackets into literal brackets", [this]()
		{
			TestEqual(TEXT("escaped"), Resolve(TEXT("He continued: \"\\[...\\] it wasn't me!")),
				FString(TEXT("He continued: \"[...] it wasn't me!")));
		});

		It("turns an escaped backslash into a single backslash", [this]()
		{
			TestEqual(TEXT("backslash"), Resolve(TEXT("a\\\\b")), FString(TEXT("a\\b")));
		});

		It("leaves a backslash before other characters alone", [this]()
		{
			TestEqual(TEXT("not an escape"), Resolve(TEXT("C:\\Temp")), FString(TEXT("C:\\Temp")));
		});

		It("does not treat an escaped bracket as the end of a token", [this]()
		{
			TestEqual(TEXT("escaped inside"), Resolve(TEXT("[if([Session.Flag], \"\\[yes\\]\", \"no\")]")), FString(TEXT("[yes]")));
		});
	});

	Describe("token scanning", [this]()
	{
		It("replaces every token in the text", [this]()
		{
			TestEqual(TEXT("two tokens"), Resolve(TEXT("[Session.PlayerName] has [Session.Score]")), FString(TEXT("Bob has 42")));
		});

		It("leaves an unterminated token alone instead of looping", [this]()
		{
			TestEqual(TEXT("no closing bracket"), Resolve(TEXT("50% off [Sale")), FString(TEXT("50% off [Sale")));
		});

		It("ignores a closing bracket that comes before the opening one", [this]()
		{
			TestEqual(TEXT("stray bracket"), Resolve(TEXT("a] [Session.PlayerName]")), FString(TEXT("a] Bob")));
		});

		It("drops an empty token", [this]()
		{
			TestEqual(TEXT("empty"), Resolve(TEXT("a[]b")), FString(TEXT("ab")));
		});

		It("resolves nested tokens innermost first", [this]()
		{
			// the Unity docs example: the inner token picks the variable name of the outer one
			TestEqual(TEXT("nested"), Resolve(TEXT("[Session.Player[Session.CurrentPlayerIndex]], its your turn!")),
				FString(TEXT("Anna, its your turn!")));
		});

		It("walks the object a string variable represents", [this]()
		{
			// the Unity docs example: a string variable holding an object representation
			TestEqual(TEXT("object from string"), Resolve(TEXT("I don't talk to a filthy [[Session.Counterpart].DisplayName]")),
				FString(TEXT("I don't talk to a filthy Hamster")));
			TestEqual(TEXT("template through string"), Resolve(TEXT("[[Session.Counterpart].Character.HP]")), FString(TEXT("7")));
		});

		It("walks an object produced by a nested token", [this]()
		{
			// nested, an object token yields its representation rather than its display name
			TestEqual(TEXT("object through object"), Resolve(TEXT("[[Chr_Manfred.Character.Companion].Character.HP]")), FString(TEXT("7")));
			TestEqual(TEXT("top level still shows the name"), Resolve(TEXT("[Chr_Manfred.Character.Companion]")), FString(TEXT("Hamster")));
		});
	});

	Describe("invalid tokens", [this]()
	{
		It("resolve to an empty string when invalid tokens are allowed", [this]()
		{
			Ext->AllowInvalidTokensOverride = true;
			TestEqual(TEXT("unknown variable"), Resolve(TEXT("Flag is [GameState.Flag].")), FString(TEXT("Flag is .")));
			TestEqual(TEXT("unknown object"), Resolve(TEXT("[Chr_Nobody.DisplayName]")), FString());
			TestEqual(TEXT("unknown property"), Resolve(TEXT("[Chr_Manfred.NoSuchProperty]")), FString());
		});

		It("hand back the whole input when invalid tokens are not allowed", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("input returned"), Resolve(TEXT("Flag is [GameState.Flag].")), FString(TEXT("Flag is [GameState.Flag].")));
			// valid tokens in the same text are not applied either
			TestEqual(TEXT("mixed"), Resolve(TEXT("[Session.PlayerName] [Nope]")), FString(TEXT("[Session.PlayerName] [Nope]")));
			// a text without errors still resolves
			TestEqual(TEXT("valid"), Resolve(TEXT("[Session.PlayerName]")), FString(TEXT("Bob")));
		});

		It("follow the Allow invalid tokens plugin setting by default", [this]()
		{
			UArticyPluginSettings* Settings = const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get());
			const bool bPrevious = Settings->bAllowInvalidTokens;

			Settings->bAllowInvalidTokens = true;
			TestEqual(TEXT("allowed"), Resolve(TEXT("a[Nope]b")), FString(TEXT("ab")));
			Settings->bAllowInvalidTokens = false;
			TestEqual(TEXT("not allowed"), Resolve(TEXT("a[Nope]b")), FString(TEXT("a[Nope]b")));

			Settings->bAllowInvalidTokens = bPrevious;
		});
	});

	Describe("literal sources", [this]()
	{
		It("inserts a quoted string as is", [this]()
		{
			TestEqual(TEXT("string"), Resolve(TEXT("[\"Hello\"]")), FString(TEXT("Hello")));
			TestEqual(TEXT("with escaped quote"), Resolve(TEXT("[\"say \\\"hi\\\"\"]")), FString(TEXT("say \"hi\"")));
		});

		It("formats a number literal", [this]()
		{
			TestEqual(TEXT("int"), Resolve(TEXT("[42]")), FString(TEXT("42")));
			TestEqual(TEXT("padded"), Resolve(TEXT("[42:000]")), FString(TEXT("042")));
			TestEqual(TEXT("float"), Resolve(TEXT("[3.14159:0.00]")), FString(TEXT("3.14")));
			TestEqual(TEXT("negative"), Resolve(TEXT("[-7]")), FString(TEXT("-7")));
		});

		It("resolves an enum value through its type", [this]()
		{
			TestEqual(TEXT("display name"), Resolve(TEXT("[CharacterClass.Mage]")), FString(TEXT("Mage")));
			TestEqual(TEXT("numeric"), Resolve(TEXT("[CharacterClass.Mage:D]")), FString(TEXT("1")));
		});
	});

	Describe("global variables", [this]()
	{
		It("resolves bool, int and string variables", [this]()
		{
			TestEqual(TEXT("bool"), Resolve(TEXT("[Session.Flag]")), FString(TEXT("true")));
			TestEqual(TEXT("int"), Resolve(TEXT("[Session.Score]")), FString(TEXT("42")));
			TestEqual(TEXT("string"), Resolve(TEXT("[Session.PlayerName]")), FString(TEXT("Bob")));
		});

		It("reflects a changed variable value", [this]()
		{
			GVs->Session->Score->Set(7);
			*GVs->Session->Flag = false;
			TestEqual(TEXT("int"), Resolve(TEXT("[Session.Score]")), FString(TEXT("7")));
			TestEqual(TEXT("bool"), Resolve(TEXT("[Session.Flag]")), FString(TEXT("false")));
		});

		It("applies a format to a variable", [this]()
		{
			TestEqual(TEXT("zero pad"), Resolve(TEXT("Highscore: [Session.Score:00000000]")), FString(TEXT("Highscore: 00000042")));
			TestEqual(TEXT("bool as number"), Resolve(TEXT("[Session.Flag:0]")), FString(TEXT("1")));
		});

		It("shows the object a string variable represents", [this]()
		{
			TestEqual(TEXT("display name"), Resolve(TEXT("[Session.Counterpart]")), FString(TEXT("Hamster")));
		});

		It("treats an unknown namespace or variable as invalid", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("namespace"), Resolve(TEXT("[Nope.Score]")), FString(TEXT("[Nope.Score]")));
			TestEqual(TEXT("variable"), Resolve(TEXT("[Session.Nope]")), FString(TEXT("[Session.Nope]")));
		});

		It("resolves nothing without global variables", [this]()
		{
			Ext->GlobalVariables = nullptr;
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("no gvs"), Resolve(TEXT("[Session.Score]")), FString(TEXT("[Session.Score]")));
		});
	});

	Describe("objects", [this]()
	{
		It("shows the display name for a bare object token", [this]()
		{
			TestEqual(TEXT("technical name"), Resolve(TEXT("Hi, [Chr_Manfred]")), FString(TEXT("Hi, Manfred")));
			TestEqual(TEXT("explicit"), Resolve(TEXT("[Chr_Manfred.DisplayName]")), FString(TEXT("Manfred")));
		});

		It("references an object by hex id, decimal id and object representation", [this]()
		{
			TestEqual(TEXT("hex"), Resolve(TEXT("[0x0100000000000002.DisplayName]")), FString(TEXT("Hamster")));
			TestEqual(TEXT("decimal"), Resolve(FString::Printf(TEXT("[%llu.DisplayName]"), HamsterId)), FString(TEXT("Hamster")));
			TestEqual(TEXT("representation"), Resolve(TEXT("[") + Representation(HamsterId) + TEXT(".DisplayName]")), FString(TEXT("Hamster")));
		});

		It("reads base properties of every basic type", [this]()
		{
			TestEqual(TEXT("int"), Resolve(TEXT("[Chr_Manfred.ZIndex]")), FString(TEXT("4")));
			TestEqual(TEXT("string"), Resolve(TEXT("[Chr_Manfred.Nickname]")), FString(TEXT("Manny")));
			TestEqual(TEXT("technical name"), Resolve(TEXT("[Chr_Manfred.TechnicalName]")), FString(TEXT("Chr_Manfred")));
		});

		It("reads template feature properties", [this]()
		{
			TestEqual(TEXT("int"), Resolve(TEXT("[Chr_Manfred.Character.HP]")), FString(TEXT("42")));
			TestEqual(TEXT("float"), Resolve(TEXT("[Chr_Manfred.Character.HitRating]")), FString(TEXT("0.42")));
			TestEqual(TEXT("bool"), Resolve(TEXT("[Chr_Manfred.Character.IsHero]")), FString(TEXT("true")));
			TestEqual(TEXT("string"), Resolve(TEXT("[Chr_Manfred.Character.Motivation]")), FString(TEXT("Revenge")));
			TestEqual(TEXT("text key without table"), Resolve(TEXT("[Chr_Manfred.Character.Title]")), FString(TEXT("Loca.Title")));
		});

		It("formats numeric properties", [this]()
		{
			TestEqual(TEXT("percent"), Resolve(TEXT("[Chr_Manfred.Character.HitRating:P]")), FString(TEXT("42%")));
			TestEqual(TEXT("padded"), Resolve(TEXT("[Chr_Manfred.Character.HP:D4]")), FString(TEXT("0042")));
		});

		It("targets an instance with angle brackets", [this]()
		{
			UArticyTextTestEntity* Clone = Cast<UArticyTextTestEntity>(DB->CloneFrom(FArticyId{ ManfredId }, 15));
			if (!TestNotNull(TEXT("clone"), Clone)) return;
			Clone->Character->HP = 99;

			TestEqual(TEXT("instance"), Resolve(TEXT("[Chr_Manfred<15>.Character.HP]")), FString(TEXT("99")));
			TestEqual(TEXT("first instance"), Resolve(TEXT("[Chr_Manfred.Character.HP]")), FString(TEXT("42")));
			TestEqual(TEXT("instance by id"), Resolve(TEXT("[0x0100000000000001<15>.Character.HP]")), FString(TEXT("99")));
		});

		It("follows a reference slot", [this]()
		{
			TestEqual(TEXT("slot object"), Resolve(TEXT("[Chr_Manfred.Character.Companion]")), FString(TEXT("Hamster")));
			TestEqual(TEXT("through slot"), Resolve(TEXT("[Chr_Manfred.Character.Companion.Character.HP]")), FString(TEXT("7")));
		});

		It("accesses reference strip elements by index, from the end and at random", [this]()
		{
			TestEqual(TEXT("first"), Resolve(TEXT("[Chr_Manfred.Character.Inventory<0>.DisplayName]")), FString(TEXT("Sword")));
			TestEqual(TEXT("second"), Resolve(TEXT("[Chr_Manfred.Character.Inventory<1>]")), FString(TEXT("Shield")));
			TestEqual(TEXT("last"), Resolve(TEXT("[Chr_Manfred.Character.Inventory<-1>]")), FString(TEXT("Shield")));

			const FString Random = Resolve(TEXT("[Chr_Manfred.Character.Inventory<?>]"));
			TestTrue(TEXT("random picks an element"), Random == TEXT("Sword") || Random == TEXT("Shield"));
			if (TestEqual(TEXT("one random pick recorded"), Ext->GetLastRandomResults().Num(), 1))
			{
				const int32 Pick = Ext->GetLastRandomResults()[0];
				TestEqual(TEXT("recorded pick matches"), Random, FString(Pick == 0 ? TEXT("Sword") : TEXT("Shield")));
			}
		});

		It("treats a strip index out of range or a strip without index as invalid", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("out of range"), Resolve(TEXT("[Chr_Manfred.Character.Inventory<5>]")), FString(TEXT("[Chr_Manfred.Character.Inventory<5>]")));
			TestEqual(TEXT("no index"), Resolve(TEXT("[Chr_Manfred.Character.Inventory]")), FString(TEXT("[Chr_Manfred.Character.Inventory]")));
		});

		It("walks the object a string property represents", [this]()
		{
			TestEqual(TEXT("display name"), Resolve(TEXT("[Chr_Manfred.Character.Partner]")), FString(TEXT("Hamster")));
			TestEqual(TEXT("through string"), Resolve(TEXT("[Chr_Manfred.Character.Partner.Character.HP]")), FString(TEXT("7")));
		});

		It("finds the database through the context object's outer chain", [this]()
		{
			// The default extension has nothing injected: with an articy object as context it
			// must still reach the database that object was loaded from, world or no world.
			UArticyTextExtension* Default = UArticyTextExtension::Get();
			TestEqual(TEXT("object by name"), Default->Resolve(Manfred, FString(TEXT("[Chr_Hamster]"))).ToString(), FString(TEXT("Hamster")));
			TestEqual(TEXT("speaker"), Default->Resolve(Fragment, FString(TEXT("[$Speaker]"))).ToString(), FString(TEXT("Manfred")));
			TestEqual(TEXT("slot through feature"), Default->Resolve(Manfred, FString(TEXT("[$Self.Character.Companion]"))).ToString(), FString(TEXT("Hamster")));
		});

		It("resolves nothing without a database", [this]()
		{
			Ext->Database = nullptr;
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("no database"), Resolve(TEXT("[Chr_Manfred]")), FString(TEXT("[Chr_Manfred]")));
		});
	});

	Describe("$Self and $Speaker", [this]()
	{
		It("resolves $Self to the context object", [this]()
		{
			TestEqual(TEXT("display name"), Resolve(TEXT("[$Self.DisplayName]"), Manfred), FString(TEXT("Manfred")));
			TestEqual(TEXT("template"), Resolve(TEXT("[$Self.Character.HP]"), Manfred), FString(TEXT("42")));
			TestEqual(TEXT("bare"), Resolve(TEXT("I am [$Self]"), Manfred), FString(TEXT("I am Manfred")));
		});

		It("resolves $Speaker to the speaker of a dialogue fragment", [this]()
		{
			TestEqual(TEXT("speaker"), Resolve(TEXT("Sorry, I only have [$Speaker.Character.HP] HP left."), Fragment),
				FString(TEXT("Sorry, I only have 42 HP left.")));
		});

		It("treats $Self and $Speaker as invalid without a fitting context", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("no context"), Resolve(TEXT("[$Self.DisplayName]"), nullptr), FString(TEXT("[$Self.DisplayName]")));
			TestEqual(TEXT("non-articy context"), Resolve(TEXT("[$Self]"), DB), FString(TEXT("[$Self]")));
			// Manfred has a Speaker property but no speaker set
			TestEqual(TEXT("no speaker"), Resolve(TEXT("[$Speaker]"), Manfred), FString(TEXT("[$Speaker]")));
		});
	});

	Describe("$Type", [this]()
	{
		It("reads type information by type name", [this]()
		{
			TestEqual(TEXT("type display name"), Resolve(TEXT("[$Type.Character.DisplayName]")), FString(TEXT("Character")));
			TestEqual(TEXT("type technical name"), Resolve(TEXT("[$Type.Character.TechnicalName]")), FString(TEXT("Character")));
			TestEqual(TEXT("has template"), Resolve(TEXT("[$Type.Character.HasTemplate]")), FString(TEXT("true")));
			TestEqual(TEXT("bare type"), Resolve(TEXT("[$Type.Character]")), FString(TEXT("Character")));
		});

		It("reads property information of a type", [this]()
		{
			TestEqual(TEXT("property display name"), Resolve(TEXT("[$Type.Character.HP.DisplayName]")), FString(TEXT("HP")));
			TestEqual(TEXT("property type"), Resolve(TEXT("[$Type.Character.HP.PropertyType]")), FString(TEXT("int")));
			TestEqual(TEXT("feature property"), Resolve(TEXT("[$Type.Character.Attributes.Strength.DisplayName]")), FString(TEXT("Attributes.Strength")));
			TestEqual(TEXT("bare property"), Resolve(TEXT("[$Type.Character.HP]")), FString(TEXT("HP")));
		});

		It("reads the type of an object instance", [this]()
		{
			// the object carries no type data of its own here (the database clone drops it), so
			// this goes through the type system's entry for the object's generated class
			TestEqual(TEXT("instance type"), Resolve(TEXT("[Chr_Manfred.$Type.DisplayName]")), FString(TEXT("Character")));
			TestEqual(TEXT("instance type name"), Resolve(TEXT("[$Self.$Type.TechnicalName]"), Manfred), FString(TEXT("Character")));
			TestEqual(TEXT("instance type property"), Resolve(TEXT("[$Self.$Type.HP]"), Manfred), FString(TEXT("HP")));
		});

		It("prefers the type's own members over a property of the same name unless the path goes on", [this]()
		{
			// every object has a DisplayName property; a bare DisplayName is the type's display name...
			TestEqual(TEXT("type display name"), Resolve(TEXT("[$Type.Character.DisplayName]")), FString(TEXT("Character")));
			// ...while a longer path addresses the property
			TestEqual(TEXT("property type"), Resolve(TEXT("[$Type.Character.DisplayName.PropertyType]")), FString(TEXT("string")));
			TestEqual(TEXT("property display name"), Resolve(TEXT("[$Type.Character.DisplayName.DisplayName]")), FString(TEXT("DisplayName")));
		});

		It("resolves type information without any context object", [this]()
		{
			TestEqual(TEXT("no context"), Ext->ResolveToken(nullptr, TEXT("$Type.Character.HP.PropertyType")), FString(TEXT("int")));
		});

		It("finds an object's type through the type system when the object carries none", [this]()
		{
			// older assets have no type data of their own; the type system knows the class
			Manfred->ArticyType = FArticyType();
			TypeSystem->Types[TEXT("Character")].CPPType = Manfred->GetClass()->GetName();
			TestEqual(TEXT("by class"), Resolve(TEXT("[Chr_Manfred.$Type.DisplayName]")), FString(TEXT("Character")));
		});

		It("resolves unknown types and members to an empty string, like the Unity plugin", [this]()
		{
			Ext->AllowInvalidTokensOverride = true;
			TestEqual(TEXT("unknown type"), Resolve(TEXT("a[$Type.Nope.DisplayName]b")), FString(TEXT("ab")));
			TestEqual(TEXT("unknown property"), Resolve(TEXT("a[$Type.Character.Nope]b")), FString(TEXT("ab")));
			TestEqual(TEXT("marker only"), Resolve(TEXT("a[$Type]b")), FString(TEXT("ab")));
			TestEqual(TEXT("property type on a value"), Resolve(TEXT("a[Chr_Manfred.Character.HP.$Type]b")), FString(TEXT("ab")));
		});

		It("treats unknown types and members as invalid", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("unknown type"), Resolve(TEXT("[$Type.Nope.DisplayName]")), FString(TEXT("[$Type.Nope.DisplayName]")));
			TestEqual(TEXT("unknown member"), Resolve(TEXT("[$Type.Character.Nope]")), FString(TEXT("[$Type.Character.Nope]")));
			TestEqual(TEXT("bare $Type"), Resolve(TEXT("[$Type]")), FString(TEXT("[$Type]")));
		});
	});

	Describe("enums", [this]()
	{
		It("shows the display name of an enum property", [this]()
		{
			TestEqual(TEXT("display name"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum]")), FString(TEXT("Bard")));
			TestEqual(TEXT("explicit display name"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum.DisplayName]")), FString(TEXT("Bard")));
			TestEqual(TEXT("technical name"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum.TechnicalName]")), FString(TEXT("Bard")));
		});

		It("shows the numeric value with the D format", [this]()
		{
			// the Unity docs example
			TestEqual(TEXT("D"), Resolve(TEXT("The enum value of [Chr_Manfred.Character.ClassEnum.DisplayName] is [Chr_Manfred.Character.ClassEnum:D]")),
				FString(TEXT("The enum value of Bard is 2")));
			TestEqual(TEXT("padded"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum:D3]")), FString(TEXT("002")));
			TestEqual(TEXT("Value member"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum.Value]")), FString(TEXT("2")));
		});

		It("falls back to the C++ enum entry name without type information", [this]()
		{
			TypeSystem->Types.Reset();
			TestEqual(TEXT("entry name"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum]")), FString(TEXT("Bard")));
			TestEqual(TEXT("still numeric"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum:D]")), FString(TEXT("2")));
		});
	});

	Describe("scripts", [this]()
	{
		It("inserts the script text of a script property", [this]()
		{
			TestEqual(TEXT("text"), Resolve(TEXT("The script text is: [Chr_Manfred.Character.IsStrong]")), FString(TEXT("The script text is: strong")));
		});

		It("evaluates a condition when called", [this]()
		{
			TestEqual(TEXT("true"), Resolve(TEXT("[Chr_Manfred.Character.IsStrong()]")), FString(TEXT("true")));
			TestEqual(TEXT("false"), Resolve(TEXT("[Chr_Hamster.Character.IsStrong()]")), FString(TEXT("false")));
			TestEqual(TEXT("as number"), Resolve(TEXT("[Chr_Manfred.Character.IsStrong():0]")), FString(TEXT("1")));
		});

		It("executes an instruction when called and inserts nothing", [this]()
		{
			UArticyTextTestExpresso* Expresso = Cast<UArticyTextTestExpresso>(DB->GetExpressoInstance());
			if (!TestNotNull(TEXT("expresso"), Expresso)) return;

			TestEqual(TEXT("nothing inserted"), Resolve(TEXT("a[Chr_Manfred.Character.Heal()]b")), FString(TEXT("ab")));
			TestEqual(TEXT("executed once"), Expresso->HealCount, 1);
		});

		It("runs a script with the owning object as self", [this]()
		{
			UArticyTextTestExpresso* Expresso = Cast<UArticyTextTestExpresso>(DB->GetExpressoInstance());
			if (!TestNotNull(TEXT("expresso"), Expresso)) return;
			Manfred->Character->IsStrong->SetExpression(TEXT("hasSelf"));

			Expresso->SetCurrentObject(nullptr);
			TestEqual(TEXT("self set during evaluation"), Resolve(TEXT("[Chr_Manfred.Character.IsStrong()]")), FString(TEXT("true")));
			TestNull(TEXT("self restored"), Expresso->self);
		});
	});

	Describe("methods", [this]()
	{
		It("picks a branch with the built-in if method", [this]()
		{
			// the Unity docs example
			TestEqual(TEXT("then"), Resolve(TEXT("Hello, how are you[if([Session.Flag], \", Officer\", \"\")]?")), FString(TEXT("Hello, how are you, Officer?")));
			*GVs->Session->Flag = false;
			TestEqual(TEXT("else"), Resolve(TEXT("Hello, how are you[if([Session.Flag], \", Officer\", \"\")]?")), FString(TEXT("Hello, how are you?")));
		});

		It("inverts the branch with the built-in not method", [this]()
		{
			TestEqual(TEXT("inverted else"), Resolve(TEXT("[not([Session.Flag], \"a\", \"b\")]")), FString(TEXT("b")));
			*GVs->Session->Flag = false;
			TestEqual(TEXT("inverted then"), Resolve(TEXT("[not([Session.Flag], \"a\", \"b\")]")), FString(TEXT("a")));
		});

		It("treats a missing else branch as an empty string", [this]()
		{
			TestEqual(TEXT("if without else"), Resolve(TEXT("x[if(false, \"yes\")]y")), FString(TEXT("xy")));
			TestEqual(TEXT("not without else"), Resolve(TEXT("x[not(true, \"yes\")]y")), FString(TEXT("xy")));
			TestEqual(TEXT("if with then only"), Resolve(TEXT("[if(true, \"yes\")]")), FString(TEXT("yes")));
		});

		It("treats a built-in with too few arguments as invalid", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("one arg"), Resolve(TEXT("[if(true)]")), FString(TEXT("[if(true)]")));
		});

		It("interprets condition values the way scripts do", [this]()
		{
			TestEqual(TEXT("1"), Resolve(TEXT("[if(1, \"y\", \"n\")]")), FString(TEXT("y")));
			TestEqual(TEXT("0"), Resolve(TEXT("[if(0, \"y\", \"n\")]")), FString(TEXT("n")));
			TestEqual(TEXT("number"), Resolve(TEXT("[if([Session.Score], \"y\", \"n\")]")), FString(TEXT("y")));
			TestEqual(TEXT("empty"), Resolve(TEXT("[if(\"\", \"y\", \"n\")]")), FString(TEXT("n")));
			TestEqual(TEXT("text"), Resolve(TEXT("[if([Session.PlayerName], \"y\", \"n\")]")), FString(TEXT("y")));
			TestEqual(TEXT("True"), Resolve(TEXT("[if(True, \"y\", \"n\")]")), FString(TEXT("y")));
		});

		It("dispatches to a registered user method with token info and arguments", [this]()
		{
			FString SeenToken;
			int32 SeenIndex = -1;
			Ext->AddUserMethod(TEXT("join"), [&](const FArticyTextToken& Token, const TArray<FString>& Args)
			{
				SeenToken = Token.Token;
				SeenIndex = Token.TokenIndex;
				return FString::Join(Args, TEXT("|"));
			});

			TestEqual(TEXT("args passed through"), Resolve(TEXT("[Session.Score] [join(a, b, c)]")), FString(TEXT("42 a|b|c")));
			TestEqual(TEXT("token source"), SeenToken, FString(TEXT("join(a, b, c)")));
			TestEqual(TEXT("token index"), SeenIndex, 1);
		});

		It("passes quoted literals, tokens and nested methods as arguments", [this]()
		{
			Ext->AddUserMethod(TEXT("join"), [](const FArticyTextToken&, const TArray<FString>& Args)
			{
				return FString::Join(Args, TEXT("|"));
			});
			Ext->AddUserMethod(TEXT("upper"), [](const FArticyTextToken&, const TArray<FString>& Args)
			{
				return Args.Num() > 0 ? Args[0].ToUpper() : FString();
			});

			TestEqual(TEXT("quoted with comma"), Resolve(TEXT("[join(\"a, b\", c)]")), FString(TEXT("a, b|c")));
			TestEqual(TEXT("token argument"), Resolve(TEXT("[join(\"now\", [Session.PlayerName])]")), FString(TEXT("now|Bob")));
			TestEqual(TEXT("nested method"), Resolve(TEXT("[join(upper(\"x\"), y)]")), FString(TEXT("X|y")));
			TestEqual(TEXT("no arguments"), Resolve(TEXT("[join()]")), FString());
		});

		It("hands the context object and parameters to a user method", [this]()
		{
			UObject* SeenContext = nullptr;
			TArray<FString> SeenParams;
			Ext->AddUserMethod(TEXT("ctx"), [&](const FArticyTextToken& Token, const TArray<FString>&)
			{
				SeenContext = Token.ContextObject;
				SeenParams = Token.Params;
				return FString(TEXT("ok"));
			});

			TestEqual(TEXT("result"), Ext->Resolve(Manfred, FString(TEXT("[ctx()]")), 7).ToString(), FString(TEXT("ok")));
			TestTrue(TEXT("context"), SeenContext == Manfred);
			TestEqual(TEXT("params"), SeenParams, TArray<FString>{ TEXT("7") });
		});

		It("still accepts the legacy argument-only callback signature", [this]()
		{
			Ext->AddUserMethod(TEXT("join"), [](const TArray<FString>& Args)
			{
				return FString::Join(Args, TEXT("|"));
			});
			TestEqual(TEXT("legacy"), Resolve(TEXT("[join(a, b)]")), FString(TEXT("a|b")));
			// the legacy [anything.method(args)] spelling keeps working for registered methods
			TestEqual(TEXT("legacy prefix"), Resolve(TEXT("[x.join(a, b, c)]")), FString(TEXT("a|b|c")));
		});

		It("removes a user method", [this]()
		{
			Ext->AddUserMethod(TEXT("now"), [](const FArticyTextToken&, const TArray<FString>&) { return FString(TEXT("12:00")); });
			TestTrue(TEXT("registered"), Ext->HasUserMethod(TEXT("now")));
			TestEqual(TEXT("called"), Resolve(TEXT("[now()]")), FString(TEXT("12:00")));

			TestTrue(TEXT("removed"), Ext->RemoveUserMethod(TEXT("now")));
			TestFalse(TEXT("gone"), Ext->HasUserMethod(TEXT("now")));
			TestFalse(TEXT("removing again"), Ext->RemoveUserMethod(TEXT("now")));

			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("unknown after removal"), Resolve(TEXT("[now()]")), FString(TEXT("[now()]")));
		});

		It("treats an unregistered method as invalid", [this]()
		{
			Ext->AllowInvalidTokensOverride = false;
			TestEqual(TEXT("unknown"), Resolve(TEXT("[nosuchmethod(a)]")), FString(TEXT("[nosuchmethod(a)]")));
		});
	});

	Describe("ResolveToken", [this]()
	{
		It("resolves a single token without brackets", [this]()
		{
			TestEqual(TEXT("variable"), Ext->ResolveToken(nullptr, TEXT("Session.Score:000")), FString(TEXT("042")));
			TestEqual(TEXT("object"), Ext->ResolveToken(nullptr, TEXT("Chr_Manfred.Character.HP")), FString(TEXT("42")));
			TestEqual(TEXT("self"), Ext->ResolveToken(Manfred, TEXT("$Self")), FString(TEXT("Manfred")));
		});

		It("returns an empty string for an unresolvable token", [this]()
		{
			TestEqual(TEXT("invalid"), Ext->ResolveToken(nullptr, TEXT("Nope.Nope")), FString());
			TestEqual(TEXT("empty"), Ext->ResolveToken(nullptr, FString()), FString());
		});

		It("resolves a token structure the way the callback receives it", [this]()
		{
			FArticyTextToken Token;
			Token.ContextObject = Manfred;
			Token.Token = TEXT("$Self.Character.HP");
			TestEqual(TEXT("from struct"), Ext->ResolveToken(Token), FString(TEXT("42")));
		});
	});

	Describe("ResolveAdvance", [this]()
	{
		It("lets the callback resolve tokens and falls back to the default for the rest", [this]()
		{
			TArray<FString> Seen;
			const FText Format = FText::FromString(TEXT("[Custom.Value] and [Session.PlayerName]"));
			const FText Result = Ext->ResolveAdvance(nullptr, Format, [&](const FArticyTextToken& Token) -> TOptional<FString>
			{
				Seen.Add(Token.Token);
				if (Token.Token == TEXT("Custom.Value"))
				{
					return FString(TEXT("custom"));
				}
				return TOptional<FString>();
			});

			TestEqual(TEXT("result"), Result.ToString(), FString(TEXT("custom and Bob")));
			TestEqual(TEXT("callback saw every token"), Seen, TArray<FString>{ TEXT("Custom.Value"), TEXT("Session.PlayerName") });
		});

		It("describes each token to the callback", [this]()
		{
			TArray<int32> Indices;
			FString Expression;
			TArray<FString> Params;
			const FText Format = FText::FromString(TEXT("{0}: [A] [B]"));
			Ext->ResolveAdvance(Manfred, Format, [&](const FArticyTextToken& Token) -> TOptional<FString>
			{
				Indices.Add(Token.TokenIndex);
				Expression = Token.Expression;
				Params = Token.Params;
				TestTrue(TEXT("context"), Token.ContextObject == Manfred);
				return FString(TEXT("x"));
			}, TEXT("p"));

			TestEqual(TEXT("indices"), Indices, TArray<int32>{ 0, 1 });
			TestEqual(TEXT("expression after params"), Expression, FString(TEXT("p: [A] [B]")));
			TestEqual(TEXT("params"), Params, TArray<FString>{ TEXT("p") });
		});

		It("can fall back to the default resolving through ResolveToken", [this]()
		{
			const FText Format = FText::FromString(TEXT("[Session.Score:000]"));
			const FText Result = Ext->ResolveAdvance(nullptr, Format, [this](const FArticyTextToken& Token) -> TOptional<FString>
			{
				return TEXT("<") + Ext->ResolveToken(Token) + TEXT(">");
			});
			TestEqual(TEXT("wrapped default"), Result.ToString(), FString(TEXT("<042>")));
		});

		It("supports the callback map overload keyed by token source", [this]()
		{
			TMap<FString, TFunction<FString(int32)>> Callbacks;
			Callbacks.Add(TEXT("Custom.Value"), [](int32 Arg) { return FString::FromInt(Arg * 2); });

			const FText Format = FText::FromString(TEXT("{0} [Custom.Value:000] [Session.PlayerName]"));
			TestEqual(TEXT("map"), Ext->ResolveAdvance(Format, Callbacks, 21).ToString(), FString(TEXT("21 42 Bob")));
		});
	});

	Describe("FormatNumber", [this]()
	{
		const auto Format = [this](const FString& Value, const FString& Fmt)
		{
			return Ext->Test_FormatNumber(Value, Fmt);
		};

		It("rounds to a whole number for the '0' format", [this, Format]()
		{
			TestEqual(TEXT("exact"), Format(TEXT("5"), TEXT("0")), FString(TEXT("5")));
			TestEqual(TEXT("rounds up"), Format(TEXT("5.7"), TEXT("0")), FString(TEXT("6")));
			TestEqual(TEXT("rounds half away from zero"), Format(TEXT("2.5"), TEXT("0")), FString(TEXT("3")));
		});

		It("zero-pads to the number of '0' digits", [this, Format]()
		{
			TestEqual(TEXT("pad"), Format(TEXT("42"), TEXT("000")), FString(TEXT("042")));
			TestEqual(TEXT("no truncation"), Format(TEXT("12345"), TEXT("000")), FString(TEXT("12345")));
		});

		It("keeps the sign in front of the zero padding", [this, Format]()
		{
			TestEqual(TEXT("negative"), Format(TEXT("-42"), TEXT("00000")), FString(TEXT("-00042")));
		});

		It("does not truncate a value beyond 32-bit range", [this, Format]()
		{
			TestEqual(TEXT("large"), Format(TEXT("5000000000"), TEXT("0")), FString(TEXT("5000000000")));
		});

		It("treats true/false as 1/0", [this, Format]()
		{
			TestEqual(TEXT("true"), Format(TEXT("true"), TEXT("0")), FString(TEXT("1")));
			TestEqual(TEXT("false"), Format(TEXT("false"), TEXT("0")), FString(TEXT("0")));
		});

		It("returns a bare 0 for a non-numeric source value", [this, Format]()
		{
			TestEqual(TEXT("not a number"), Format(TEXT("GameState.Flag"), TEXT("0")), FString(TEXT("0")));
		});

		It("passes through literal characters in the format", [this, Format]()
		{
			TestEqual(TEXT("suffix"), Format(TEXT("5"), TEXT("0 pts")), FString(TEXT("5 pts")));
			TestEqual(TEXT("prefix"), Format(TEXT("5"), TEXT("$0")), FString(TEXT("$5")));
			TestEqual(TEXT("quoted"), Format(TEXT("5"), TEXT("0 '#1'")), FString(TEXT("5 #1")));
		});

		It("masks digits with '#' placeholders", [this, Format]()
		{
			// the Unity docs example
			TestEqual(TEXT("mask"), Format(TEXT("123456789"), TEXT("###-###-###")), FString(TEXT("123-456-789")));
			// as in C#, literals are kept even where no digit lands in front of them
			TestEqual(TEXT("mask with fewer digits"), Format(TEXT("12345"), TEXT("###-###-###")), FString(TEXT("-12-345")));
			TestEqual(TEXT("mask with more digits"), Format(TEXT("1234567890"), TEXT("###-###-###")), FString(TEXT("1234-567-890")));
		});

		It("handles decimals with '0' and '#' after the point", [this, Format]()
		{
			TestEqual(TEXT("fixed"), Format(TEXT("3.14159"), TEXT("0.00")), FString(TEXT("3.14")));
			TestEqual(TEXT("optional decimals"), Format(TEXT("3.5"), TEXT("0.00##")), FString(TEXT("3.50")));
			TestEqual(TEXT("trailing optional dropped"), Format(TEXT("3"), TEXT("0.##")), FString(TEXT("3")));
			TestEqual(TEXT("integer part optional"), Format(TEXT("0.5"), TEXT("#.00")), FString(TEXT(".50")));
			TestEqual(TEXT("'#' alone keeps the integer"), Format(TEXT("3.14159"), TEXT("##")), FString(TEXT("3")));
		});

		It("groups thousands and scales percentages in custom formats", [this, Format]()
		{
			TestEqual(TEXT("grouping"), Format(TEXT("1234567"), TEXT("#,##0")), FString(TEXT("1,234,567")));
			TestEqual(TEXT("percent"), Format(TEXT("0.42"), TEXT("0%")), FString(TEXT("42%")));
			TestEqual(TEXT("percent decimals"), Format(TEXT("0.4256"), TEXT("0.0%")), FString(TEXT("42.6%")));
		});

		It("picks the section for negative and zero values", [this, Format]()
		{
			TestEqual(TEXT("positive"), Format(TEXT("5"), TEXT("0;(0);zero")), FString(TEXT("5")));
			TestEqual(TEXT("negative"), Format(TEXT("-5"), TEXT("0;(0);zero")), FString(TEXT("(5)")));
			TestEqual(TEXT("zero"), Format(TEXT("0"), TEXT("0;(0);zero")), FString(TEXT("zero")));
		});

		It("supports the standard numeric formats", [this, Format]()
		{
			TestEqual(TEXT("D"), Format(TEXT("42"), TEXT("D")), FString(TEXT("42")));
			TestEqual(TEXT("D5"), Format(TEXT("42"), TEXT("D5")), FString(TEXT("00042")));
			TestEqual(TEXT("D negative"), Format(TEXT("-42"), TEXT("D4")), FString(TEXT("-0042")));
			TestEqual(TEXT("F"), Format(TEXT("3.14159"), TEXT("F")), FString(TEXT("3.14")));
			TestEqual(TEXT("F1"), Format(TEXT("3.14159"), TEXT("F1")), FString(TEXT("3.1")));
			TestEqual(TEXT("N"), Format(TEXT("1234567.891"), TEXT("N")), FString(TEXT("1,234,567.89")));
			TestEqual(TEXT("N0"), Format(TEXT("1234567.891"), TEXT("N0")), FString(TEXT("1,234,568")));
			TestEqual(TEXT("P"), Format(TEXT("0.42"), TEXT("P")), FString(TEXT("42%")));
			TestEqual(TEXT("P1"), Format(TEXT("0.4256"), TEXT("P1")), FString(TEXT("42.6%")));
			TestEqual(TEXT("X"), Format(TEXT("255"), TEXT("X")), FString(TEXT("FF")));
			TestEqual(TEXT("x4"), Format(TEXT("255"), TEXT("x4")), FString(TEXT("00ff")));
		});
	});

	Describe("ResolveBoolean", [this]()
	{
		It("falls back to \"true\"/\"false\" when no localizer is present", [this]()
		{
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

	// These need the string table and the test localizer; they are skipped with a warning
	// when a generated localizer shadows the test one.
	Describe("localization", [this]()
	{
		It("localizes a string value that is a loca key", [this]()
		{
			if (!BeginLocalization({ { TEXT("Loca.Title"), TEXT("Lord of Hamsters") } })) return;

			TestEqual(TEXT("text property"), Resolve(TEXT("[Chr_Manfred.Character.Title]")), FString(TEXT("Lord of Hamsters")));
			TestEqual(TEXT("string variable"), Resolve(TEXT("[Session.Title]")), FString(TEXT("Lord of Hamsters")));
			TestEqual(TEXT("literal"), Resolve(TEXT("[\"Loca.Title\"]")), FString(TEXT("Lord of Hamsters")));
			TestEqual(TEXT("plain string stays"), Resolve(TEXT("[Session.PlayerName]")), FString(TEXT("Bob")));
		});

		It("localizes an object's display name", [this]()
		{
			if (!BeginLocalization({ { TEXT("Manfred"), TEXT("Manfred the Great") } })) return;
			TestEqual(TEXT("display name"), Resolve(TEXT("[Chr_Manfred]")), FString(TEXT("Manfred the Great")));
		});

		It("resolves tokens inside a localized value", [this]()
		{
			if (!BeginLocalization({ { TEXT("Loca.Title"), TEXT("Lord of [Session.Counterpart]") } })) return;
			TestEqual(TEXT("nested in loca"), Resolve(TEXT("[Session.Title]")), FString(TEXT("Lord of Hamster")));
		});

		It("localizes the input itself when it is a loca key", [this]()
		{
			if (!BeginLocalization({ { TEXT("Loca.Greeting"), TEXT("Hello [Session.PlayerName]") } })) return;
			TestEqual(TEXT("key input"), Resolve(TEXT("Loca.Greeting")), FString(TEXT("Hello Bob")));
			TestEqual(TEXT("key input via FText"), Ext->ResolveToken(nullptr, TEXT("\"Loca.Greeting\"")), FString(TEXT("Hello Bob")));
		});

		It("uses localized boolean texts", [this]()
		{
			if (!BeginLocalization({
				{ TEXT("VariableConstants.Boolean.True"), TEXT("Yes") },
				{ TEXT("VariableConstants.Boolean.False"), TEXT("No") },
				{ TEXT("Session.Flag.True"), TEXT("Flagged") } })) return;

			TestEqual(TEXT("per-source text"), Resolve(TEXT("[Session.Flag]")), FString(TEXT("Flagged")));
			TestEqual(TEXT("constant"), Resolve(TEXT("[Chr_Manfred.Character.IsHero]")), FString(TEXT("Yes")));
			*GVs->Session->Flag = false;
			TestEqual(TEXT("constant false"), Resolve(TEXT("[Session.Flag]")), FString(TEXT("No")));
			// a localized boolean still drives if/not
			TestEqual(TEXT("if on localized"), Resolve(TEXT("[if([Chr_Manfred.Character.IsHero], \"hero\", \"nobody\")]")), FString(TEXT("hero")));
			TestEqual(TEXT("if on localized false"), Resolve(TEXT("[if([Session.Flag], \"hero\", \"nobody\")]")), FString(TEXT("nobody")));
		});

		It("localizes type and enum display names", [this]()
		{
			if (!BeginLocalization({ { TEXT("Character"), TEXT("Charakter") }, { TEXT("Bard"), TEXT("Barde") } })) return;
			TestEqual(TEXT("type"), Resolve(TEXT("[$Type.Character.DisplayName]")), FString(TEXT("Charakter")));
			TestEqual(TEXT("enum"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum]")), FString(TEXT("Barde")));
			TestEqual(TEXT("enum technical name unchanged"), Resolve(TEXT("[Chr_Manfred.Character.ClassEnum.TechnicalName]")), FString(TEXT("Bard")));
		});
	});

	Describe("Resolve strings setting", [this]()
	{
		It("controls whether object texts resolve their tokens automatically", [this]()
		{
			if (!BeginLocalization({})) return;

			UArticyPluginSettings* Settings = const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get());
			const bool bPrevious = Settings->bResolveStrings;

			// Reading through the interface goes through the default text extension, which has
			// no database in this test; an escape sequence shows whether resolving happened.
			Manfred->DisplayName = FText::FromString(TEXT("Manfred \\[the Great\\]"));
			IArticyObjectWithDisplayName* WithDisplayName = Cast<IArticyObjectWithDisplayName>(Manfred);
			if (!TestNotNull(TEXT("interface"), WithDisplayName)) return;

			Settings->bResolveStrings = true;
			TestEqual(TEXT("resolved"), WithDisplayName->GetDisplayName().ToString(), FString(TEXT("Manfred [the Great]")));

			Settings->bResolveStrings = false;
			TestEqual(TEXT("raw"), WithDisplayName->GetDisplayName().ToString(), FString(TEXT("Manfred \\[the Great\\]")));

			Settings->bResolveStrings = bPrevious;
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
