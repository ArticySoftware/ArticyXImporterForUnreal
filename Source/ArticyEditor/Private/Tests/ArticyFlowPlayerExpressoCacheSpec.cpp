//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ArticyDatabase.h"
#include "ArticyExpressoScripts.h"
#include "ArticyFlowPlayer.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

#if WITH_AUTOMATION_TESTS

// Integration tests: need a host project that has already imported articy content (the
// ManiacManfred demo), since resolving the methods provider needs the generated expresso class.
// The category deliberately avoids the "Articy" substring so the unit runner ("Automation
// RunTests Articy") does not pick them up.
namespace
{
	UWorld* GetExpressoCacheWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	FObjectProperty* FindFlowPlayerExpressoCacheProperty()
	{
		return CastField<FObjectProperty>(UArticyFlowPlayer::StaticClass()->FindPropertyByName(TEXT("CachedExpressoInstance")));
	}
}

// UArticyFlowPlayer::GetMethodsProvider caches the database's expresso instance. The cache must
// hold the right instance, and must let go of one that has become garbage instead of using it.
BEGIN_DEFINE_SPEC(FArticyFlowPlayerExpressoCacheSpec, "AXImporter.Integration.FlowPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	UArticyDatabase* DB = nullptr;
	AActor* Actor = nullptr;
	UArticyFlowPlayer* Player = nullptr;

	// Hosts a flow player in the editor world. Returns false when the test cannot run; the
	// reason has been reported already. TearDownPlayer() is safe after a failed setup.
	bool SetUpPlayer();
	void TearDownPlayer();

END_DEFINE_SPEC(FArticyFlowPlayerExpressoCacheSpec)

bool FArticyFlowPlayerExpressoCacheSpec::SetUpPlayer()
{
	UWorld* World = GetExpressoCacheWorld();
	if (!TestNotNull(TEXT("editor world"), World))
		return false;

	DB = UArticyDatabase::Get(World);
	if (!TestNotNull(TEXT("database"), DB))
		return false;

	Actor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("actor"), Actor))
		return false;

	Player = NewObject<UArticyFlowPlayer>(Actor);
	Player->RegisterComponent();
	return true;
}

void FArticyFlowPlayerExpressoCacheSpec::TearDownPlayer()
{
	if (Actor)
		Actor->Destroy();

	DB = nullptr;
	Actor = nullptr;
	Player = nullptr;
}

void FArticyFlowPlayerExpressoCacheSpec::Define()
{
	Describe("GetMethodsProvider", [this]()
	{
		It("caches the database's expresso instance", [this]()
		{
			ON_SCOPE_EXIT { TearDownPlayer(); };
			if (!SetUpPlayer())
				return;

			FObjectProperty* Cache = FindFlowPlayerExpressoCacheProperty();
			if (!TestNotNull(TEXT("CachedExpressoInstance is a UPROPERTY"), Cache))
				return;

			Player->GetMethodsProvider();

			TestTrue(TEXT("cached the database's instance"),
				Cache->GetObjectPropertyValue_InContainer(Player) == DB->GetExpressoInstance());
		});

		It("fetches the instance again once the cached one is garbage", [this]()
		{
			ON_SCOPE_EXIT { TearDownPlayer(); };
			if (!SetUpPlayer())
				return;

			FObjectProperty* Cache = FindFlowPlayerExpressoCacheProperty();
			if (!TestNotNull(TEXT("CachedExpressoInstance is a UPROPERTY"), Cache))
				return;

			UArticyExpressoScripts* Real = DB->GetExpressoInstance();
			if (!TestNotNull(TEXT("database expresso instance"), Real))
				return;

			// Stand in for an instance whose database clone has been collected: same class as the
			// real one, owned by nobody, and already marked as garbage. A plain null check would
			// still accept it, since GC does not null references to garbage objects.
			UArticyExpressoScripts* Stale = NewObject<UArticyExpressoScripts>(GetTransientPackage(), Real->GetClass());
			Cache->SetObjectPropertyValue_InContainer(Player, Stale);
			Stale->MarkAsGarbage();

			Player->GetMethodsProvider();

			TestTrue(TEXT("replaced the garbage instance with the database's"),
				Cache->GetObjectPropertyValue_InContainer(Player) == Real);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
