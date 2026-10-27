//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ArticyFlowPlayer.h"
#include "ArticyTestExpresso.h"
#include "ArticyTestFlowListener.h"
#include "ArticyBuiltinTypes.h"
#include "ArticyPins.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_AUTOMATION_TESTS

// Everything the flow player can be checked on without imported articy content: the shape of
// its setup, the callbacks it broadcasts and the garbage collection of the expresso instance it
// caches. The traversal itself needs a real flow, which AXImporter.Integration covers.
BEGIN_DEFINE_SPEC(FArticyFlowPlayerSpec, "Articy.Runtime.FlowPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyFlowPlayerSpec)

void FArticyFlowPlayerSpec::Define()
{
	// The cached instance is owned by a database clone which only static weak pointers track, so
	// the cache has to be a reference the garbage collector follows, or it dangles once the clone is gone.
	Describe("CachedExpressoInstance", [this]()
	{
		It("is a reflected property that keeps the cached instance alive through garbage collection", [this]()
		{
			FObjectProperty* Cache = CastField<FObjectProperty>(UArticyFlowPlayer::StaticClass()->FindPropertyByName(TEXT("CachedExpressoInstance")));
			if (!TestNotNull(TEXT("CachedExpressoInstance is a UPROPERTY"), Cache))
				return;

			// Root the player, so only the cached instance's lifetime is under test.
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->AddToRoot();
			ON_SCOPE_EXIT { Player->RemoveFromRoot(); };

			// Nothing but the player's cache references this instance.
			TWeakObjectPtr<UArticyTestExpresso> Expresso = NewObject<UArticyTestExpresso>(GetTransientPackage());
			Cache->SetObjectPropertyValue_InContainer(Player, Expresso.Get());

			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
			TestTrue(TEXT("cached instance survives GC while the player holds it"), Expresso.IsValid());

			// Counter-check: it was the player's reference that kept it alive, nothing else.
			Cache->SetObjectPropertyValue_InContainer(Player, nullptr);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
			TestFalse(TEXT("cached instance is collected once released"), Expresso.IsValid());
		});
	});

	Describe("PauseOn", [this]()
	{
		// EArticyPausableType values are bit indices, so Connection (the ninth) needs a flag
		// of 256 - which no longer fits the uint8 the bitmask used to be stored in.
		It("is wide enough to hold every pausable type", [this]()
		{
			FProperty* Property = UArticyFlowPlayer::StaticClass()->FindPropertyByName(TEXT("PauseOn"));
			if (!TestNotNull(TEXT("PauseOn is a UPROPERTY"), Property))
				return;

			TestNotNull(TEXT("PauseOn is an int32"), CastField<FIntProperty>(Property));

			const int32 HighestFlag = ArticyPauseOnFlag(EArticyPausableType::Connection);
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->SetPauseOnMask(HighestFlag);

			TestEqual(TEXT("the connection flag round-trips"), Player->GetPauseOn(), HighestFlag);
			TestTrue(TEXT("and is reported as set"), Player->PausesOnType(EArticyPausableType::Connection));
		});

		It("sets exactly the requested single type", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->SetPauseOn(EArticyPausableType::Hub);

			TestEqual(TEXT("only the hub flag is set"), Player->GetPauseOn(), ArticyPauseOnFlag(EArticyPausableType::Hub));
			TestTrue(TEXT("pauses on hubs"), Player->PausesOnType(EArticyPausableType::Hub));
			TestFalse(TEXT("no longer pauses on the default dialogue fragments"), Player->PausesOnType(EArticyPausableType::DialogueFragment));
		});

		It("keeps every type of a bitmask", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->SetPauseOnMask(ArticyPauseOnFlag(EArticyPausableType::Pin) | ArticyPauseOnFlag(EArticyPausableType::Connection));

			TestTrue(TEXT("pauses on pins"), Player->PausesOnType(EArticyPausableType::Pin));
			TestTrue(TEXT("pauses on connections"), Player->PausesOnType(EArticyPausableType::Connection));
			TestFalse(TEXT("does not pause on jumps"), Player->PausesOnType(EArticyPausableType::Jump));
		});
	});

	Describe("Connections", [this]()
	{
		It("are flow objects that report themselves as connections", [this]()
		{
			UArticyOutgoingConnection* Connection = NewObject<UArticyOutgoingConnection>(GetTransientPackage());

			IArticyFlowObject* FlowObject = Cast<IArticyFlowObject>(Connection);
			if (!TestNotNull(TEXT("a connection is a flow object"), FlowObject))
				return;

			TestEqual(TEXT("its pausable type"), static_cast<uint8>(FlowObject->GetType()), static_cast<uint8>(EArticyPausableType::Connection));
		});

		It("are only paused on when the connection flag is set", [this]()
		{
			UArticyOutgoingConnection* Connection = NewObject<UArticyOutgoingConnection>(GetTransientPackage());
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());

			Player->SetPauseOn(EArticyPausableType::DialogueFragment);
			TestFalse(TEXT("not paused on by default"), Player->ShouldPauseOn(Connection));

			Player->SetPauseOn(EArticyPausableType::Connection);
			TestTrue(TEXT("paused on once requested"), Player->ShouldPauseOn(Connection));
		});
	});

	Describe("Continuous branch recalculation", [this]()
	{
		It("is off by default", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());

			TestFalse(TEXT("disabled"), Player->bContinuouslyRecalculateBranches);
			TestFalse(TEXT("never fires while disabled"), Player->AdvanceRecalculationTimer(100.f));
		});

		It("fires once the interval has passed and then starts over", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->bContinuouslyRecalculateBranches = true;
			Player->ContinuouslyRecalculateInterval = 1.f;

			TestFalse(TEXT("0.4s in"), Player->AdvanceRecalculationTimer(0.4f));
			TestFalse(TEXT("0.8s in"), Player->AdvanceRecalculationTimer(0.4f));
			TestTrue(TEXT("1.2s in"), Player->AdvanceRecalculationTimer(0.4f));
			TestFalse(TEXT("0.4s after the last recalculation"), Player->AdvanceRecalculationTimer(0.4f));
		});
	});

	Describe("Callbacks", [this]()
	{
		It("can be triggered manually", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			UArticyTestFlowListener* Listener = NewObject<UArticyTestFlowListener>(GetTransientPackage());

			Player->OnPlayerPaused.AddDynamic(Listener, &UArticyTestFlowListener::HandlePlayerPaused);
			Player->OnBranchesUpdated.AddDynamic(Listener, &UArticyTestFlowListener::HandleBranchesUpdated);

			Player->BroadcastPlayerPaused();
			Player->BroadcastBranchesUpdated();

			TestEqual(TEXT("paused once"), Listener->PausedCount, 1);
			TestEqual(TEXT("branches reported once"), Listener->BranchesUpdatedCount, 1);
			TestEqual(TEXT("with the current, empty branches"), Listener->LastBranchCount, 0);
		});
	});

	Describe("ValidateSetup", [this]()
	{
		It("reports a missing start node", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());

			const TArray<FArticyFlowPlayerSetupIssue> Issues = Player->ValidateSetup();
			const FArticyFlowPlayerSetupIssue* StartOnIssue = Issues.FindByPredicate(
				[](const FArticyFlowPlayerSetupIssue& Issue) { return Issue.PropertyName == TEXT("StartOn"); });

			if (!TestNotNull(TEXT("an issue about StartOn"), StartOnIssue))
				return;

			// A warning, not an error: a start node set at runtime is a legitimate setup
			TestEqual(TEXT("reported as a warning"), static_cast<uint8>(StartOnIssue->Severity), static_cast<uint8>(EArticyFlowPlayerIssueSeverity::Warning));
			TestFalse(TEXT("with a message"), StartOnIssue->Message.IsEmpty());
		});

		It("reports an empty PauseOn", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			Player->SetPauseOnMask(0);

			const TArray<FArticyFlowPlayerSetupIssue> Issues = Player->ValidateSetup();
			TestTrue(TEXT("an issue about PauseOn"), Issues.ContainsByPredicate(
				[](const FArticyFlowPlayerSetupIssue& Issue) { return Issue.PropertyName == TEXT("PauseOn"); }));
		});

		It("says nothing about PauseOn while it has types set", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());

			const TArray<FArticyFlowPlayerSetupIssue> Issues = Player->ValidateSetup();
			TestFalse(TEXT("no issue about PauseOn"), Issues.ContainsByPredicate(
				[](const FArticyFlowPlayerSetupIssue& Issue) { return Issue.PropertyName == TEXT("PauseOn"); }));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
