//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyExpressoScripts.h"
#include "ArticyObjectNotificationManager.h"
#include "ArticyTestNotificationObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	/** Sets a property through the reflectable interface. */
	void SetInt(UArticyBaseObject* Object, const TCHAR* Property, int32 Value)
	{
		Object->SetProp<int32>(Property, Value);
	}

	/** The flags as the bitmask a Blueprint passes. */
	int32 Mask(EArticyTypeProperties Flags)
	{
		return static_cast<int32>(Flags);
	}
}

// Pins who is told about which change: by object, by type, by property filter and by flags.
BEGIN_DEFINE_SPEC(FArticyObjectNotificationManagerSpec, "Articy.Runtime.ObjectNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UArticyDatabase* DB = nullptr;
	UArticyObjectNotificationManager* Manager = nullptr;
	UArticyTestNotifyCharacter* Manfred = nullptr;
	UArticyTestNotifyCharacter* Clara = nullptr;
	UArticyTestNotifyEntity* Barrel = nullptr;
	UArticyTestNotifyLocation* Cellar = nullptr;

	/** The properties reported to Record(), in order. */
	TArray<FString> Reported;
	/** The object last reported to Record(). */
	UArticyBaseObject* ReportedObject = nullptr;

	/** A listener filling Reported and ReportedObject. */
	FArticyPropertyChangedFunction Record();
	/** Reported as a comma separated list. */
	FString ReportedList() const { return FString::Join(Reported, TEXT(",")); }
END_DEFINE_SPEC(FArticyObjectNotificationManagerSpec)

FArticyPropertyChangedFunction FArticyObjectNotificationManagerSpec::Record()
{
	return [this](const FArticyChangedProperty& Changed)
	{
		Reported.Add(Changed.Property.ToString());
		ReportedObject = Changed.ObjectReference;
	};
}

void FArticyObjectNotificationManagerSpec::Define()
{
	// A change finds its manager through the database the object is outered to
	BeforeEach([this]()
	{
		DB = NewObject<UArticyDatabase>(GetTransientPackage());
		Manager = DB->GetObjectNotifications();

		Manfred = NewObject<UArticyTestNotifyCharacter>(DB);
		Manfred->CreateFeature();
		Clara = NewObject<UArticyTestNotifyCharacter>(DB);
		Clara->CreateFeature();
		Barrel = NewObject<UArticyTestNotifyEntity>(DB);
		Cellar = NewObject<UArticyTestNotifyLocation>(DB);

		Reported.Reset();
		ReportedObject = nullptr;
	});

	Describe("The manager of a database", [this]()
	{
		It("is the same every time it is asked for", [this]()
		{
			TestNotNull(TEXT("manager"), Manager);
			TestTrue(TEXT("same manager"), DB->GetObjectNotifications() == Manager);
		});

		It("is not shared with another database", [this]()
		{
			UArticyDatabase* Other = NewObject<UArticyDatabase>(GetTransientPackage());
			UArticyTestNotifyEntity* Stranger = NewObject<UArticyTestNotifyEntity>(Other);
			Manager->AddListener(TEXT("*"), Record());

			SetInt(Stranger, TEXT("Level"), 2);

			TestTrue(TEXT("own manager"), Other->GetObjectNotifications() != Manager);
			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("is not created by a change nobody listens to", [this]()
		{
			UArticyDatabase* Other = NewObject<UArticyDatabase>(GetTransientPackage());
			UArticyTestNotifyEntity* Stranger = NewObject<UArticyTestNotifyEntity>(Other);

			SetInt(Stranger, TEXT("Level"), 2);

			TestNull(TEXT("manager"), Other->FindObjectNotifications());
		});
	});

	Describe("A listener for an object", [this]()
	{
		It("is told the object and the property that changed", [this]()
		{
			Manager->AddListener(Manfred, Record());

			SetInt(Manfred, TEXT("Level"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level")));
			TestTrue(TEXT("reported object"), ReportedObject == Manfred);
		});

		It("is not told about other objects", [this]()
		{
			Manager->AddListener(Manfred, Record());

			SetInt(Clara, TEXT("Level"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("is only told about the properties matching its filter", [this]()
		{
			Manager->AddListener(Manfred, TEXT("Courage"), Record());

			SetInt(Manfred, TEXT("Level"), 3);
			SetInt(Manfred, TEXT("Courage"), 4);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage")));
		});

		It("treats a clone as an object of its own", [this]()
		{
			UArticyCloneableObject* Cloneable = NewObject<UArticyCloneableObject>(DB);
			Cloneable->Init(Manfred);
			UArticyObject* Clone = Cloneable->Clone(DB, 1);
			Manager->AddListener(Clone, Record());

			SetInt(Manfred, TEXT("Level"), 3);
			TestEqual(TEXT("reported for the original"), ReportedList(), FString());

			SetInt(Clone, TEXT("Level"), 5);
			TestEqual(TEXT("reported for the clone"), ReportedList(), FString(TEXT("Level")));
			TestTrue(TEXT("reported object"), ReportedObject == Clone);
		});

		It("is refused without an object", [this]()
		{
			AddExpectedError(TEXT("Cannot add a listener"), EAutomationExpectedErrorFlags::Contains, 1);

			const UArticyBaseObject* Nobody = nullptr;
			TestFalse(TEXT("handle is valid"), Manager->AddListener(Nobody, Record()).IsValid());
			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 0);
		});
	});

	Describe("A listener for a type", [this]()
	{
		It("is told about every object of the class", [this]()
		{
			Manager->AddListener(UArticyTestNotifyCharacter::StaticClass(), TEXT("*"), Record());

			SetInt(Manfred, TEXT("Courage"), 1);
			SetInt(Clara, TEXT("Level"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage,Level")));
		});

		It("is told about objects of derived classes", [this]()
		{
			Manager->AddListener<UArticyTestNotifyEntity>(TEXT("Level"), Record());

			SetInt(Barrel, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Level"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level,Level")));
		});

		It("is not told about objects of other classes", [this]()
		{
			Manager->AddListener<UArticyTestNotifyCharacter>(TEXT("*"), Record());

			SetInt(Barrel, TEXT("Level"), 1);
			SetInt(Cellar, TEXT("Size"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("accepts an interface for all objects implementing it", [this]()
		{
			Manager->AddListener(UArticyObjectWithDisplayName::StaticClass(), TEXT("*"), Record());

			SetInt(Barrel, TEXT("Level"), 1);
			SetInt(Cellar, TEXT("Size"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level")));
		});

		It("accepts an articy type for the class generated for it", [this]()
		{
			FArticyType CharacterType;
			CharacterType.TechnicalName = TEXT("Character");
			CharacterType.CPPType = TEXT("U") + UArticyTestNotifyCharacter::StaticClass()->GetName();
			const FArticyListenerHandle Handle = Manager->AddListener(CharacterType, TEXT("*"), Record());

			SetInt(Manfred, TEXT("Courage"), 1);
			SetInt(Barrel, TEXT("Level"), 2);

			TestTrue(TEXT("handle is valid"), Handle.IsValid());
			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage")));
		});

		It("is refused for an articy type no class is generated for", [this]()
		{
			AddExpectedError(TEXT("no class is generated for it"), EAutomationExpectedErrorFlags::Contains, 1);

			FArticyType UnknownType;
			UnknownType.TechnicalName = TEXT("Unknown");
			UnknownType.CPPType = TEXT("UArticyTestNoSuchClass");

			TestFalse(TEXT("handle is valid"), Manager->AddListener(UnknownType, TEXT("*"), Record()).IsValid());
			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 0);
		});
	});

	Describe("The filter", [this]()
	{
		It("matches every property when it is a wildcard", [this]()
		{
			Manager->AddListener(TEXT("*"), Record());

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Cellar, TEXT("Size"), 2);
			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level,Size,Morale.MoraleValue")));
		});

		It("matches every property when it is empty", [this]()
		{
			Manager->AddListener(FString(), Record());

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Cellar, TEXT("Size"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level,Size")));
		});

		It("matches a property by name on any object", [this]()
		{
			Manager->AddListener(TEXT("Level"), Record());

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Courage"), 2);
			SetInt(Barrel, TEXT("Level"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level,Level")));
		});

		It("matches a template property by feature and property", [this]()
		{
			Manager->AddListener(TEXT("Morale.MoraleValue"), Record());

			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);
			Manfred->Morale->SetProp<FString>(TEXT("Mood"), TEXT("Grim"));

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.MoraleValue")));
			TestTrue(TEXT("reported object is the owner of the feature"), ReportedObject == Manfred);
		});

		It("matches all properties of a feature with a wildcard", [this]()
		{
			Manager->AddListener(TEXT("Morale.*"), Record());

			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);
			Manfred->Morale->SetProp<FString>(TEXT("Mood"), TEXT("Grim"));
			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.MoraleValue,Morale.Mood")));
		});

		It("does not match a template property by its property name alone", [this]()
		{
			Manager->AddListener(TEXT("MoraleValue"), Record());

			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("ignores case", [this]()
		{
			Manager->AddListener(TEXT("morale.moralevalue"), Record());

			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);

			TestEqual(TEXT("reported"), Reported.Num(), 1);
		});
	});

	Describe("The flags", [this]()
	{
		It("limit a listener to template properties", [this]()
		{
			Manager->AddListener(TEXT("*"), EArticyTypeProperties::Template, Record());

			SetInt(Manfred, TEXT("Courage"), 1);
			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.MoraleValue")));
		});

		It("limit a listener to the general properties of the type itself", [this]()
		{
			Manager->AddListener(TEXT("*"), EArticyTypeProperties::General, Record());

			// Level belongs to the entity: inherited by the character, the barrel's own
			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Courage"), 2);
			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);
			SetInt(Barrel, TEXT("Level"), 4);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage,Level")));
			TestTrue(TEXT("last reported object"), ReportedObject == Barrel);
		});

		It("include the properties of the base type when asked to", [this]()
		{
			Manager->AddListener(TEXT("*"), EArticyTypeProperties::General | EArticyTypeProperties::IncludeBaseType, Record());

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Courage"), 2);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Level,Courage")));
		});

		It("limit a listener to the properties of the articy object itself", [this]()
		{
			Manager->AddListener(TEXT("*"), EArticyTypeProperties::ArticyObject, Record());

			SetInt(Manfred, TEXT("Level"), 1);
			static_cast<UArticyBaseObject*>(Manfred)->SetProp<FString>(TEXT("TechnicalName"), TEXT("Chr_Manfred"));

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("TechnicalName")));
		});

		It("apply to listeners for an object and for a type as well", [this]()
		{
			Manager->AddListener(Manfred, TEXT("*"), EArticyTypeProperties::Template, Record());
			Manager->AddListener<UArticyTestNotifyCharacter>(TEXT("*"), EArticyTypeProperties::Template, Record());

			SetInt(Manfred, TEXT("Courage"), 1);
			SetInt(Manfred->Morale, TEXT("MoraleValue"), 3);

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.MoraleValue,Morale.MoraleValue")));
		});
	});

	Describe("A change", [this]()
	{
		It("is reported when a script sets a property", [this]()
		{
			Manager->AddListener(TEXT("*"), Record());

			// What setProp in a script comes down to
			ExpressoType(7).SetValue(Manfred, TEXT("Courage"));

			TestEqual(TEXT("value"), Manfred->Courage, 7);
			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage")));
			TestTrue(TEXT("reported object"), ReportedObject == Manfred);
		});

		It("is reported when a script sets a template property", [this]()
		{
			Manager->AddListener(TEXT("*"), Record());

			ExpressoType(9).SetValue(Manfred, TEXT("Morale.MoraleValue"));

			TestEqual(TEXT("value"), Manfred->Morale->MoraleValue, 9);
			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.MoraleValue")));
			TestTrue(TEXT("reported object"), ReportedObject == Manfred);
		});

		It("can be reported by hand after assigning a property directly", [this]()
		{
			Manager->AddListener(Manfred, TEXT("Courage"), Record());

			Manfred->Courage = 5;
			Manager->ReportChanged(Manfred, TEXT("Courage"));

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Courage")));
		});

		It("can be reported by hand for a feature", [this]()
		{
			Manager->AddListener(Manfred, TEXT("Morale.*"), Record());

			Manager->ReportChanged(Manfred->Morale, TEXT("Mood"));
			Manager->ReportChanged(Manfred, TEXT("Morale.MoraleValue"));

			TestEqual(TEXT("reported"), ReportedList(), FString(TEXT("Morale.Mood,Morale.MoraleValue")));
			TestTrue(TEXT("reported object"), ReportedObject == Manfred);
		});

		It("still reaches the delegate of the object itself", [this]()
		{
			int32 Broadcasts = 0;
			static_cast<UArticyBaseObject*>(Manfred)->ReportChanged.AddLambda([&Broadcasts](FArticyChangedProperty&) { ++Broadcasts; });

			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("broadcasts"), Broadcasts, 1);
		});

		It("is not reported for an object outside of a database", [this]()
		{
			UArticyTestNotifyEntity* Asset = NewObject<UArticyTestNotifyEntity>(GetTransientPackage());
			Manager->AddListener(TEXT("*"), Record());

			SetInt(Asset, TEXT("Level"), 1);

			TestEqual(TEXT("reported"), ReportedList(), FString());
		});
	});

	Describe("Removing", [this]()
	{
		It("a listener by its handle stops its notifications", [this]()
		{
			const FArticyListenerHandle Handle = Manager->AddListener(TEXT("*"), Record());

			TestTrue(TEXT("removed"), Manager->RemoveListener(Handle));
			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("reported"), ReportedList(), FString());
			TestFalse(TEXT("removed again"), Manager->RemoveListener(Handle));
		});

		It("a listener keeps the others", [this]()
		{
			int32 Kept = 0;
			const FArticyListenerHandle Handle = Manager->AddListener(TEXT("*"), Record());
			Manager->AddListener(TEXT("*"), [&Kept](const FArticyChangedProperty&) { ++Kept; });

			Manager->RemoveListener(Handle);
			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("kept listener called"), Kept, 1);
			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("nothing for a handle that was never handed out", [this]()
		{
			Manager->AddListener(TEXT("*"), Record());

			TestFalse(TEXT("removed"), Manager->RemoveListener(FArticyListenerHandle()));
			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 1);
		});

		It("the listeners of an object keeps those of other objects and of types", [this]()
		{
			Manager->AddListener(Manfred, Record());
			Manager->AddListener(Manfred, TEXT("Level"), Record());
			Manager->AddListener(Clara, Record());
			Manager->AddListener<UArticyTestNotifyCharacter>(TEXT("*"), Record());

			TestEqual(TEXT("removed"), Manager->RemoveListeners(Manfred), 2);
			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 2);
			TestEqual(TEXT("reported to the type listener only"), ReportedList(), FString(TEXT("Level")));
		});

		It("the listeners of a type keeps those of other types and of objects", [this]()
		{
			Manager->AddListener<UArticyTestNotifyCharacter>(TEXT("*"), Record());
			Manager->AddListener<UArticyTestNotifyEntity>(TEXT("*"), Record());
			Manager->AddListener(Manfred, Record());

			TestEqual(TEXT("removed"), Manager->RemoveListeners<UArticyTestNotifyCharacter>(), 1);

			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 2);
		});

		It("all listeners leaves nobody to tell", [this]()
		{
			Manager->AddListener(TEXT("*"), Record());
			Manager->AddListener(Manfred, Record());
			Manager->AddListener<UArticyTestNotifyCharacter>(TEXT("*"), Record());

			Manager->RemoveAllListeners();
			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 0);
			TestEqual(TEXT("reported"), ReportedList(), FString());
		});

		It("a listener from within its own notification is safe", [this]()
		{
			int32 Calls = 0;
			TSharedRef<FArticyListenerHandle> Handle = MakeShared<FArticyListenerHandle>();
			*Handle = Manager->AddListener(TEXT("*"), [this, Handle, &Calls](const FArticyChangedProperty&)
			{
				++Calls;
				Manager->RemoveListener(*Handle);
			});
			Manager->AddListener(TEXT("*"), Record());

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Level"), 2);

			TestEqual(TEXT("calls of the removed listener"), Calls, 1);
			TestEqual(TEXT("reported to the other listener"), ReportedList(), FString(TEXT("Level,Level")));
		});

		It("all listeners from within a notification skips the ones not called yet", [this]()
		{
			Manager->AddListener(TEXT("*"), [this](const FArticyChangedProperty&) { Manager->RemoveAllListeners(); });
			Manager->AddListener(TEXT("*"), Record());

			SetInt(Manfred, TEXT("Level"), 1);

			TestEqual(TEXT("reported"), ReportedList(), FString());
		});
	});

	Describe("A Blueprint listener", [this]()
	{
		It("is called for any object", [this]()
		{
			UArticyTestNotificationListener* Listener = NewObject<UArticyTestNotificationListener>(GetTransientPackage());
			FArticyPropertyChangedDelegate OnChanged;
			OnChanged.BindUFunction(Listener, GET_FUNCTION_NAME_CHECKED(UArticyTestNotificationListener, HandleChanged));
			const FArticyListenerHandle Handle = Manager->AddPropertyListener(TEXT("Level"), OnChanged);

			SetInt(Manfred, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Courage"), 2);

			TestTrue(TEXT("handle is valid"), Handle.IsValid());
			TestEqual(TEXT("reported"), FString::Join(Listener->Properties, TEXT(",")), FString(TEXT("Level")));
			TestTrue(TEXT("reported object"), Listener->LastObject == Manfred);
		});

		It("is called for its object only", [this]()
		{
			UArticyTestNotificationListener* Listener = NewObject<UArticyTestNotificationListener>(GetTransientPackage());
			FArticyPropertyChangedDelegate OnChanged;
			OnChanged.BindUFunction(Listener, GET_FUNCTION_NAME_CHECKED(UArticyTestNotificationListener, HandleChanged));
			Manager->AddObjectListener(Manfred, TEXT("*"), OnChanged);

			SetInt(Clara, TEXT("Level"), 1);
			SetInt(Manfred, TEXT("Level"), 2);

			TestEqual(TEXT("reported"), Listener->Properties.Num(), 1);
			TestTrue(TEXT("reported object"), Listener->LastObject == Manfred);
			TestEqual(TEXT("removed"), Manager->RemoveObjectListeners(Manfred), 1);
		});

		It("is called for its type with the flags of its bitmask", [this]()
		{
			UArticyTestNotificationListener* Listener = NewObject<UArticyTestNotificationListener>(GetTransientPackage());
			FArticyPropertyChangedDelegate OnChanged;
			OnChanged.BindUFunction(Listener, GET_FUNCTION_NAME_CHECKED(UArticyTestNotificationListener, HandleChanged));
			Manager->AddTypeListener(UArticyTestNotifyCharacter::StaticClass(), TEXT("*"), OnChanged, Mask(EArticyTypeProperties::Template));

			SetInt(Manfred, TEXT("Courage"), 1);
			SetInt(Manfred->Morale, TEXT("MoraleValue"), 2);
			SetInt(Barrel, TEXT("Level"), 3);

			TestEqual(TEXT("reported"), FString::Join(Listener->Properties, TEXT(",")), FString(TEXT("Morale.MoraleValue")));
			TestEqual(TEXT("removed"), Manager->RemoveTypeListeners(UArticyTestNotifyCharacter::StaticClass()), 1);
		});

		It("is refused without an event", [this]()
		{
			AddExpectedError(TEXT("Cannot add a listener"), EAutomationExpectedErrorFlags::Contains, 1);

			TestFalse(TEXT("handle is valid"), Manager->AddPropertyListener(TEXT("*"), FArticyPropertyChangedDelegate()).IsValid());
			TestEqual(TEXT("listeners"), Manager->GetNumListeners(), 0);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
