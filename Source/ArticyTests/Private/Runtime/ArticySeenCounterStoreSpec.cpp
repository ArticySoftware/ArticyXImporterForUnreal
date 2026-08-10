//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyGlobalVariables.h"
#include "ArticyTestFlowObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// A store plus two distinguishable nodes. Built inside each It rather than held on the
	// spec, so the objects cannot be collected between two queued test commands.
	struct FSeenFixture
	{
		FSeenFixture()
			: GVs(NewObject<UArticyGlobalVariables>())
			, NodeA(NewObject<UArticyTestFlowObject>())
			, NodeB(NewObject<UArticyTestFlowObject>())
		{
			NodeA->SetTestId(FArticyId{ uint64(0x0100000000000001) });
			NodeB->SetTestId(FArticyId{ uint64(0x0100000000000002) });
		}

		UArticyGlobalVariables* GVs;
		UArticyTestFlowObject* NodeA;
		UArticyTestFlowObject* NodeB;
	};
}

// The seen-counter store on UArticyGlobalVariables is what "seen", "unseen" and
// "seenCounter" ultimately read and write, and PushSeen/PopSeen is what keeps a shadowed
// exploration from leaking its writes into the real state. Both are only reachable through
// a flow player in normal use, so this exercises the store directly.
BEGIN_DEFINE_SPEC(FArticySeenCounterStoreSpec, "Articy.Runtime.SeenCounterStore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticySeenCounterStoreSpec)

void FArticySeenCounterStoreSpec::Define()
{
	Describe("counters", [this]()
	{
		It("reports 0 for a node that was never seen", [this]()
		{
			FSeenFixture F;
			TestEqual(TEXT("unseen"), F.GVs->GetSeenCounter(F.NodeA), 0);
		});

		It("stores and reads back a counter per node", [this]()
		{
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 3);
			TestEqual(TEXT("A"), F.GVs->GetSeenCounter(F.NodeA), 3);
			TestEqual(TEXT("B untouched"), F.GVs->GetSeenCounter(F.NodeB), 0);
		});

		It("overwrites an existing counter rather than adding a second entry", [this]()
		{
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 3);
			TestEqual(TEXT("set returns the new value"), F.GVs->SetSeenCounter(F.NodeA, 8), 8);
			TestEqual(TEXT("A"), F.GVs->GetSeenCounter(F.NodeA), 8);
		});

		It("increments from nothing to 1 and then upwards", [this]()
		{
			FSeenFixture F;
			TestEqual(TEXT("first visit"), F.GVs->IncrementSeenCounter(F.NodeA), 1);
			TestEqual(TEXT("second visit"), F.GVs->IncrementSeenCounter(F.NodeA), 2);
			TestEqual(TEXT("A"), F.GVs->GetSeenCounter(F.NodeA), 2);
		});

		It("keys on the id, so two objects sharing an id share a counter", [this]()
		{
			// Clones of the same articy object carry the same id; the store does not
			// distinguish them, which is why a cloned node inherits its original's count.
			FSeenFixture F;
			F.NodeB->SetTestId(F.NodeA->GetId());
			F.GVs->SetSeenCounter(F.NodeA, 5);
			TestEqual(TEXT("same id, same counter"), F.GVs->GetSeenCounter(F.NodeB), 5);
		});

		It("returns 0 for a null object instead of asserting", [this]()
		{
			// getSeenCounter(NameOrId) can fail to resolve an object, so the store has to
			// tolerate a null target.
			FSeenFixture F;
			TestEqual(TEXT("get"), F.GVs->GetSeenCounter(nullptr), 0);
			TestEqual(TEXT("set"), F.GVs->SetSeenCounter(nullptr, 4), 0);
			TestEqual(TEXT("increment"), F.GVs->IncrementSeenCounter(nullptr), 0);
		});

		It("clears every counter on ResetVisited", [this]()
		{
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 3);
			F.GVs->SetSeenCounter(F.NodeB, 4);
			F.GVs->ResetVisited();
			TestEqual(TEXT("A"), F.GVs->GetSeenCounter(F.NodeA), 0);
			TestEqual(TEXT("B"), F.GVs->GetSeenCounter(F.NodeB), 0);
		});
	});

	Describe("PushSeen / PopSeen", [this]()
	{
		It("carries the current counters into the pushed state", [this]()
		{
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 3);
			F.GVs->PushSeen();
			TestEqual(TEXT("visible inside"), F.GVs->GetSeenCounter(F.NodeA), 3);
		});

		It("discards writes made while pushed", [this]()
		{
			// This is what keeps a shadowed exploration from marking nodes as seen: the
			// player pushes, explores, then pops the whole layer away.
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 3);
			F.GVs->PushSeen();
			F.GVs->SetSeenCounter(F.NodeA, 99);
			F.GVs->IncrementSeenCounter(F.NodeB);
			F.GVs->PopSeen();
			TestEqual(TEXT("A restored"), F.GVs->GetSeenCounter(F.NodeA), 3);
			TestEqual(TEXT("B never happened"), F.GVs->GetSeenCounter(F.NodeB), 0);
		});

		It("nests", [this]()
		{
			FSeenFixture F;
			F.GVs->SetSeenCounter(F.NodeA, 1);
			F.GVs->PushSeen();
			F.GVs->SetSeenCounter(F.NodeA, 2);
			F.GVs->PushSeen();
			F.GVs->SetSeenCounter(F.NodeA, 3);
			TestEqual(TEXT("innermost"), F.GVs->GetSeenCounter(F.NodeA), 3);
			F.GVs->PopSeen();
			TestEqual(TEXT("middle"), F.GVs->GetSeenCounter(F.NodeA), 2);
			F.GVs->PopSeen();
			TestEqual(TEXT("outermost"), F.GVs->GetSeenCounter(F.NodeA), 1);
		});

		It("drops the base layer when the pair straddles the very first write", [this]()
		{
			// Asymmetry worth knowing about: PushSeen only pushes when there is already a
			// layer to copy, but PopSeen always pops. On a store that has never been
			// written to (no ResetVisited yet), the write below creates the base layer and
			// the matching PopSeen then removes it, so the count is lost.
			FSeenFixture F;
			F.GVs->PushSeen();
			F.GVs->SetSeenCounter(F.NodeA, 7);
			F.GVs->PopSeen();
			TestEqual(TEXT("count did not survive"), F.GVs->GetSeenCounter(F.NodeA), 0);
		});

		It("keeps the pair balanced once a layer exists", [this]()
		{
			// ResetVisited seeds the base layer, after which push/pop is symmetric.
			FSeenFixture F;
			F.GVs->ResetVisited();
			F.GVs->PushSeen();
			F.GVs->SetSeenCounter(F.NodeA, 7);
			F.GVs->PopSeen();
			F.GVs->SetSeenCounter(F.NodeB, 2);
			TestEqual(TEXT("shadowed write discarded"), F.GVs->GetSeenCounter(F.NodeA), 0);
			TestEqual(TEXT("base layer still there"), F.GVs->GetSeenCounter(F.NodeB), 2);
		});

		It("survives a pop with nothing pushed", [this]()
		{
			FSeenFixture F;
			F.GVs->PopSeen();
			TestEqual(TEXT("still readable"), F.GVs->GetSeenCounter(F.NodeA), 0);
		});
	});

	Describe("fallback evaluation", [this]()
	{
		It("is false for a node that was never flagged", [this]()
		{
			FSeenFixture F;
			TestFalse(TEXT("not flagged"), F.GVs->Fallback(F.NodeA));
		});

		It("stores and reads back a flag per node", [this]()
		{
			FSeenFixture F;
			F.GVs->SetFallbackEvaluation(F.NodeA, true);
			TestTrue(TEXT("A"), F.GVs->Fallback(F.NodeA));
			TestFalse(TEXT("B untouched"), F.GVs->Fallback(F.NodeB));
		});

		It("clears a flag again", [this]()
		{
			FSeenFixture F;
			F.GVs->SetFallbackEvaluation(F.NodeA, true);
			F.GVs->SetFallbackEvaluation(F.NodeA, false);
			TestFalse(TEXT("A"), F.GVs->Fallback(F.NodeA));
		});

		It("treats a null object as \"is any node in fallback\"", [this]()
		{
			// The null overload is the global query used to decide whether a fallback
			// branch is currently being evaluated at all.
			FSeenFixture F;
			TestFalse(TEXT("nothing flagged"), F.GVs->Fallback(nullptr));
			F.GVs->SetFallbackEvaluation(F.NodeA, true);
			TestTrue(TEXT("some node flagged"), F.GVs->Fallback(nullptr));
		});

		It("is shadowed alongside the counters", [this]()
		{
			FSeenFixture F;
			F.GVs->SetFallbackEvaluation(F.NodeA, true);
			F.GVs->PushSeen();
			F.GVs->SetFallbackEvaluation(F.NodeA, false);
			F.GVs->PopSeen();
			TestTrue(TEXT("flag restored"), F.GVs->Fallback(F.NodeA));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
