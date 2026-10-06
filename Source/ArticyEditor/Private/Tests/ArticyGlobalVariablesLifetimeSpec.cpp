//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ArticyGlobalVariables.h"
#include "ArticyPluginSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

// Integration tests: GetDefault clones the project's global variables asset, so these need a host
// project that has already imported articy content. The category deliberately avoids the "Articy"
// substring so the unit runner ("Automation RunTests Articy") does not pick them up.
//
// A runtime clone must live exactly as long as its outer. It must not owe its life to RF_Standalone
// or the root set: the editor keeps RF_Standalone objects alive and a packaged game does not, and a
// rooted clone outlives its outer. So each test checks that the clone has neither, that it survives a
// garbage collection all the same - something references it, in any build - and that it is collected
// once its outer is gone.
BEGIN_DEFINE_SPEC(FArticyGlobalVariablesLifetimeSpec, "AXImporter.Integration.GlobalVariables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	UWorld* World = nullptr;
	bool bOriginalKeepBetweenWorlds = true;

	// Creates a bare game world for GetDefault to clone into, with the given "keep global variables
	// between worlds" setting. Returns false when the test cannot run; the reason has been reported
	// already. TearDown() is safe after a failed setup.
	bool SetUp(bool bKeepBetweenWorlds);
	void DestroyWorld();
	void TearDown();

	void TestSurvivesGarbageCollection(UArticyGlobalVariables* RuntimeClone);

END_DEFINE_SPEC(FArticyGlobalVariablesLifetimeSpec)

bool FArticyGlobalVariablesLifetimeSpec::SetUp(bool bKeepBetweenWorlds)
{
	UArticyGlobalVariables* Original = UArticyGlobalVariables::GetMutableOriginal();
	if (!Original)
	{
		AddWarning(TEXT("Skipped: this project has no imported articy global variables."));
		return false;
	}

	// GetDefault hands out the existing clone while there is one; drop it so the test clones afresh
	Original->UnloadGlobalVariables();

	// UArticyPluginSettings::Get() is a transient copy of the project settings, so this saves nothing
	UArticyPluginSettings* Settings = const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get());
	bOriginalKeepBetweenWorlds = Settings->bKeepGlobalVariablesBetweenWorlds;
	Settings->bKeepGlobalVariablesBetweenWorlds = bKeepBetweenWorlds;

	World = UWorld::CreateWorld(EWorldType::Game, false);
	return TestNotNull(TEXT("test world"), World);
}

void FArticyGlobalVariablesLifetimeSpec::DestroyWorld()
{
	if (!World)
		return;

	World->SetGameInstance(nullptr);
	World->DestroyWorld(false);
	World = nullptr;
}

void FArticyGlobalVariablesLifetimeSpec::TearDown()
{
	// Unload while the outer is still there to release the clone; the editor then clones afresh
	if (UArticyGlobalVariables* Original = UArticyGlobalVariables::GetMutableOriginal())
		Original->UnloadGlobalVariables();

	DestroyWorld();

	const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get())->bKeepGlobalVariablesBetweenWorlds = bOriginalKeepBetweenWorlds;
}

void FArticyGlobalVariablesLifetimeSpec::TestSurvivesGarbageCollection(UArticyGlobalVariables* RuntimeClone)
{
	TestFalse(TEXT("not kept alive by RF_Standalone, which only the editor honors"), RuntimeClone->HasAnyFlags(RF_Standalone));
	TestFalse(TEXT("not a GC root"), RuntimeClone->IsRooted());

	const TWeakObjectPtr<UArticyGlobalVariables> WeakClone = RuntimeClone;
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestTrue(TEXT("survives garbage collection"), WeakClone.IsValid());
}

void FArticyGlobalVariablesLifetimeSpec::Define()
{
	Describe("Runtime clone lifetime", [this]()
	{
		It("keeps a persistent clone for as long as its game instance", [this]()
		{
			ON_SCOPE_EXIT { TearDown(); };
			if (!SetUp(true))
				return;

			TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GEngine));
			World->SetGameInstance(GameInstance.Get());

			UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GV))
				return;

			TestTrue(TEXT("cloned into the game instance"), GV->GetOuter() == GameInstance.Get());
			TestSurvivesGarbageCollection(GV);

			const TWeakObjectPtr<UArticyGlobalVariables> WeakGV = GV;
			World->SetGameInstance(nullptr);
			GameInstance.Reset();
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			TestFalse(TEXT("collected together with the game instance"), WeakGV.IsValid());
		});

		It("keeps a per-world clone for as long as its world", [this]()
		{
			ON_SCOPE_EXIT { TearDown(); };
			if (!SetUp(false))
				return;

			UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GV))
				return;

			TestTrue(TEXT("cloned into the world"), GV->GetOuter() == World);
			TestSurvivesGarbageCollection(GV);

			// Cleaning the world up deinitializes its subsystems, the one that owns the clone among them
			const TWeakObjectPtr<UArticyGlobalVariables> WeakGV = GV;
			DestroyWorld();
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			TestFalse(TEXT("collected together with the world"), WeakGV.IsValid());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
