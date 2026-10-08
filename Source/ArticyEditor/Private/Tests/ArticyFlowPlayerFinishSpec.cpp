//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ArticyDatabase.h"
#include "ArticyFlowPlayer.h"
#include "ArticyObject.h"
#include "ArticyPins.h"
#include "Interfaces/ArticyOutputPinsProvider.h"
#include "ArticyCountingTestExpresso.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

#if WITH_AUTOMATION_TESTS

// Integration tests: need a host project that has already imported articy content (the
// ManiacManfred demo). The category deliberately avoids the "Articy" substring so the unit
// runner ("Automation RunTests Articy") does not pick them up.
namespace
{
	const TCHAR* FinishDemoDialogue = TEXT("Dlg_TheTherapist");

	UWorld* GetFlowPlayerWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	// The database caches its expresso instance in a private property without a setter, so the
	// only way to observe executions is to swap that instance through reflection.
	FObjectProperty* FindDatabaseExpressoCacheProperty()
	{
		return CastField<FObjectProperty>(UArticyDatabase::StaticClass()->FindPropertyByName(TEXT("CachedExpressoScripts")));
	}
}

// UArticyFlowPlayer::FinishCurrentPausedObject runs the script on one output pin of the
// paused object. Instructions are not idempotent in general (counters, toggles), so running
// a pin once must mean exactly once.
BEGIN_DEFINE_SPEC(FArticyFlowPlayerFinishSpec, "AXImporter.Integration.FlowPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	AActor* Actor = nullptr;
	UArticyFlowPlayer* Player = nullptr;
	UArticyOutputPin* Pin = nullptr;
	int32 PinCount = 0;
	FString OriginalPinText;
	UArticyDatabase* PinDB = nullptr;
	UArticyExpressoScripts* OriginalExpresso = nullptr;
	UArticyCountingTestExpresso* Counting = nullptr;

	// Pauses a flow player on the demo dialogue and routes the paused object's first output pin
	// to the counting expresso. Returns false when the test cannot run; the reason has been
	// reported already. TearDownPausedPin() is safe to call after a failed setup.
	bool SetUpPausedPin();
	void TearDownPausedPin();

END_DEFINE_SPEC(FArticyFlowPlayerFinishSpec)

bool FArticyFlowPlayerFinishSpec::SetUpPausedPin()
{
	UWorld* World = GetFlowPlayerWorld();
	if (!TestNotNull(TEXT("editor world"), World))
		return false;

	UArticyDatabase* DB = UArticyDatabase::Get(World);
	if (!TestNotNull(TEXT("database"), DB))
		return false;

	UArticyObject* StartNode = DB->GetObjectByName(FName(FinishDemoDialogue));
	if (!StartNode)
	{
		AddWarning(FString::Printf(TEXT("Skipped: '%s' is not in this project's imported articy content."), FinishDemoDialogue));
		return false;
	}

	Actor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("actor"), Actor))
		return false;

	Player = NewObject<UArticyFlowPlayer>(Actor);
	Player->RegisterComponent();

	// Explore with the project's real expresso, so the pins and conditions on the way to the
	// first pause find their fragments. This also leaves the player paused on a dialogue
	// fragment, which is the situation FinishCurrentPausedObject is meant for.
	Player->SetStartNodeById(StartNode->GetId());

	// The player caches the expresso instance it resolves its methods provider from. Prime that
	// cache with the real one now, because the counting stand-in has no provider interface.
	Player->GetMethodsProvider();

	IArticyOutputPinsProvider* PinOwner = Cast<IArticyOutputPinsProvider>(Player->GetCursor().GetObject());
	if (!TestNotNull(TEXT("paused object provides output pins"), PinOwner))
		return false;

	const TArray<UArticyOutputPin*>* Pins = PinOwner->GetOutputPinsPtr();
	if (!TestTrue(TEXT("paused object has an output pin"), Pins && Pins->Num() > 0))
		return false;

	Pin = (*Pins)[0];
	PinCount = Pins->Num();

	// UArticyOutputPin::Execute resolves its database via UArticyDatabase::Get(this), so the
	// swap has to happen on that database, not necessarily the one Get(World) returns.
	PinDB = UArticyDatabase::Get(Pin);
	if (!TestNotNull(TEXT("pin database"), PinDB))
		return false;

	FObjectProperty* Cache = FindDatabaseExpressoCacheProperty();
	if (!TestNotNull(TEXT("CachedExpressoScripts property"), Cache))
		return false;

	Counting = NewObject<UArticyCountingTestExpresso>(PinDB);
	Counting->Init(PinDB);

	OriginalExpresso = Cast<UArticyExpressoScripts>(Cache->GetObjectPropertyValue_InContainer(PinDB));
	Cache->SetObjectPropertyValue_InContainer(PinDB, Counting);

	OriginalPinText = Pin->Text;
	Pin->Text = UArticyCountingTestExpresso::CountedFragment();
	return true;
}

void FArticyFlowPlayerFinishSpec::TearDownPausedPin()
{
	if (Pin)
		Pin->Text = OriginalPinText;

	if (PinDB && Counting)
	{
		if (FObjectProperty* Cache = FindDatabaseExpressoCacheProperty())
			Cache->SetObjectPropertyValue_InContainer(PinDB, OriginalExpresso);
	}

	if (Actor)
		Actor->Destroy();

	Actor = nullptr;
	Player = nullptr;
	Pin = nullptr;
	PinCount = 0;
	OriginalPinText.Reset();
	PinDB = nullptr;
	OriginalExpresso = nullptr;
	Counting = nullptr;
}

void FArticyFlowPlayerFinishSpec::Define()
{
	Describe("FinishCurrentPausedObject", [this]()
	{
		It("executes the chosen output pin's instruction exactly once", [this]()
		{
			ON_SCOPE_EXIT { TearDownPausedPin(); };
			if (!SetUpPausedPin())
				return;

			Counting->ExecutionCount = 0;
			Player->FinishCurrentPausedObject(0);

			TestEqual(TEXT("executions"), Counting->ExecutionCount, 1);
		});

		It("executes nothing for an out-of-range pin index", [this]()
		{
			ON_SCOPE_EXIT { TearDownPausedPin(); };
			if (!SetUpPausedPin())
				return;

			AddExpectedError(TEXT("FinishCurrentPausedObject: The index was out of bounds"), EAutomationExpectedErrorFlags::Contains, 1);

			Counting->ExecutionCount = 0;
			Player->FinishCurrentPausedObject(PinCount);

			TestEqual(TEXT("executions"), Counting->ExecutionCount, 0);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
