//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ArticyFlowPlayer.h"
#include "ArticyTestExpresso.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_AUTOMATION_TESTS

// The flow player caches the expresso instance it resolves its methods provider from. That
// instance is owned by a database clone which only static weak pointers track, so the cache
// has to be a reference the garbage collector follows, or it dangles once the clone is gone.
BEGIN_DEFINE_SPEC(FArticyFlowPlayerSpec, "Articy.Runtime.FlowPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyFlowPlayerSpec)

void FArticyFlowPlayerSpec::Define()
{
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
}

#endif // WITH_AUTOMATION_TESTS
