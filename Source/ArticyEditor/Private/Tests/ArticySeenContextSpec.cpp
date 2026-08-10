//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyExpressoScripts.h"
#include "ArticyObject.h"
#include "ArticyPins.h"
#include "Interfaces/ArticyInputPinsProvider.h"
#include "ArticySeenTestExpresso.h"
#include "Editor.h"
#include "Engine/World.h"

#if WITH_AUTOMATION_TESTS

// Integration test: requires a host project that has already imported articy content
// (the ManiacManfred demo). The category deliberately avoids the "Articy" substring so
// the unit runner ("Automation RunTests Articy") does not pick it up.
namespace
{
	UWorld* GetSeenContextWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}
}

BEGIN_DEFINE_SPEC(FArticySeenContextSpec, "AXImporter.Integration.SeenContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticySeenContextSpec)

void FArticySeenContextSpec::Define()
{
	Describe("current object (self) for pin scripts", [this]()
	{
		It("resolves to the pin, so a pin script cannot see its owning node's seen counter", [this]()
		{
			UWorld* World = GetSeenContextWorld();
			if (!TestNotNull(TEXT("editor world"), World)) return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB)) return;

			UArticyGlobalVariables* GVs = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GVs)) return;

			UArticyObject* Node = DB->GetObjectByName(FName(TEXT("FFr_Lobby")));
			if (!TestNotNull(TEXT("FFr_Lobby"), Node)) return;

			IArticyInputPinsProvider* Provider = Cast<IArticyInputPinsProvider>(Node);
			if (!TestNotNull(TEXT("input pin provider"), Provider)) return;

			const TArray<UArticyInputPin*>* Pins = Provider->GetInputPinsPtr();
			if (!TestTrue(TEXT("node has an input pin"), Pins && Pins->Num() > 0)) return;

			UArticyInputPin* Pin = (*Pins)[0];
			if (!TestNotNull(TEXT("input pin"), Pin)) return;

			// UArticyInputPin::Evaluate resolves its own database via UArticyDatabase::Get(this),
			// so take the expresso instance from the same place or we would be inspecting a
			// different object than the one Evaluate actually touches.
			UArticyDatabase* PinDB = UArticyDatabase::Get(Pin);
			UArticyExpressoScripts* Xp = PinDB ? PinDB->GetExpressoInstance() : nullptr;
			if (!TestNotNull(TEXT("expresso instance"), Xp)) return;

			AddInfo(FString::Printf(TEXT("DB=%s PinDB=%s sameDB=%d sameXp=%d GVclass=%s"),
				*GetNameSafe(DB), *GetNameSafe(PinDB), DB == PinDB ? 1 : 0,
				(DB && Xp == DB->GetExpressoInstance()) ? 1 : 0,
				*GetNameSafe(GVs->GetClass())));

			// A pin and its owner are distinct objects, tracked under separate ids. Which one
			// 'self' points at therefore decides what a pin's script can see.
			// (Read through the GVs directly: UArticyExpressoScripts::Evaluate calls
			// SetGV(nullptr) on its way out, so getSeenCounter() only resolves a GV while a
			// fragment is actually executing and would return 0 for everything out here.)
			GVs->SetSeenCounter(Cast<IArticyFlowObject>(Node), 7);
			GVs->SetSeenCounter(Cast<IArticyFlowObject>(Pin), 0);
			TestEqual(TEXT("node counter is 7"), GVs->GetSeenCounter(Cast<IArticyFlowObject>(Node)), 7);
			TestEqual(TEXT("pin counter is 0, independent of its owner"),
				GVs->GetSeenCounter(Cast<IArticyFlowObject>(Pin)), 0);

			// UArticyInputPin::Evaluate never assigns 'self' - it inherits whatever
			// UArticyFlowPlayer::Explore last set, which for a pin is the pin itself.
			// Evaluate clears GV and UserMethodsProvider on exit, but deliberately not self.
			Xp->SetCurrentObject(Cast<UArticyPrimitive>(Node));
			Pin->Evaluate(GVs, nullptr);
			TestTrue(TEXT("Evaluate leaves self untouched (never resolves the pin's owner)"),
				Xp->self == Cast<UArticyPrimitive>(Node));

			// ...and it does not set self to anything at all, not even the pin.
			Xp->SetCurrentObject(nullptr);
			Pin->Evaluate(GVs, nullptr);
			TestNull(TEXT("Evaluate does not assign self"), Xp->self);
		});

		It("evaluates a bare seen condition on an input pin against the owning node", [this]()
		{
			UWorld* World = GetSeenContextWorld();
			if (!TestNotNull(TEXT("editor world"), World)) return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB)) return;

			UArticyGlobalVariables* GVs = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GVs)) return;

			UArticyObject* Node = DB->GetObjectByName(FName(TEXT("FFr_Lobby")));
			if (!TestNotNull(TEXT("FFr_Lobby"), Node)) return;

			IArticyInputPinsProvider* Provider = Cast<IArticyInputPinsProvider>(Node);
			if (!TestNotNull(TEXT("input pin provider"), Provider)) return;

			const TArray<UArticyInputPin*>* Pins = Provider->GetInputPinsPtr();
			if (!TestTrue(TEXT("node has an input pin"), Pins && Pins->Num() > 0)) return;

			UArticyInputPin* Pin = (*Pins)[0];
			if (!TestNotNull(TEXT("input pin"), Pin)) return;

			UArticySeenTestExpresso* Xp = NewObject<UArticySeenTestExpresso>();
			if (!TestNotNull(TEXT("fixture expresso"), Xp)) return;

			// self is the pin while a pin's condition runs - this is what
			// UArticyFlowPlayer::Explore does before handing over to the pin.
			Xp->SetCurrentObject(Pin);
			GVs->SetSeenCounter(Cast<IArticyFlowObject>(Pin), 0);

			// The node has been seen; "seen" on its input pin must therefore be true.
			GVs->SetSeenCounter(Cast<IArticyFlowObject>(Node), 5);
			TestTrue(TEXT("owning node seen 5x -> seen is true"),
				Xp->Evaluate(UArticySeenTestExpresso::SeenFragmentHash(), GVs, nullptr));

			// ...and false again once the node is unseen, so the check above cannot pass
			// just because the condition always returns true.
			GVs->SetSeenCounter(Cast<IArticyFlowObject>(Node), 0);
			TestFalse(TEXT("owning node unseen -> seen is false"),
				Xp->Evaluate(UArticySeenTestExpresso::SeenFragmentHash(), GVs, nullptr));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
