//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyObject.h"
#include "ArticyTextExtension.h"
#include "ArticyFlowPlayer.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "Engine/World.h"

#if WITH_AUTOMATION_TESTS

// Integration tests: these require a host project that has already imported articy
// content (a generated database + global variables), e.g. the ManiacManfred demo
// project. They only touch the plugin's base-class API, so the plugin never depends
// on the generated game module.
namespace
{
	UWorld* GetIntegrationWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}
}

// Category deliberately avoids the "Articy" substring so the unit runner (which runs
// "Automation RunTests Articy", a substring match) does not pick these up.
BEGIN_DEFINE_SPEC(FArticyIntegrationSpec, "AXImporter.Integration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyIntegrationSpec)

void FArticyIntegrationSpec::Define()
{
	Describe("Database", [this]()
	{
		It("loads the imported database and exposes objects", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			const TArray<UArticyObject*> Objects = DB->GetObjectsOfClass(UArticyObject::StaticClass());
			TestTrue(TEXT("database has objects"), Objects.Num() > 0);
		});

		It("finds a known object by its technical name", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Lobby = DB->GetObjectByName(FName(TEXT("FFr_Lobby")));
			TestNotNull(TEXT("FFr_Lobby found"), Lobby);
		});
	});

	Describe("GlobalVariables", [this]()
	{
		It("reads a known boolean variable from the imported GVs", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GV))
				return;

			bool bSucceeded = false;
			GV->GetBoolVariable(FArticyGvName(FName(TEXT("GameState")), FName(TEXT("awake"))), bSucceeded);
			TestTrue(TEXT("GameState.awake resolved"), bSucceeded);
		});

		It("sets and reads back a boolean variable", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GV))
				return;

			const FArticyGvName Awake(FName(TEXT("GameState")), FName(TEXT("awake")));
			bool bOk = false;
			const bool bOriginal = GV->GetBoolVariable(Awake, bOk);

			GV->SetBoolVariable(Awake, true);
			TestTrue(TEXT("reads true after set"), GV->GetBoolVariable(Awake, bOk));

			GV->SetBoolVariable(Awake, false);
			TestFalse(TEXT("reads false after set"), GV->GetBoolVariable(Awake, bOk));

			GV->SetBoolVariable(Awake, bOriginal); // restore
		});
	});

	Describe("Text resolution", [this]()
	{
		It("resolves a [Namespace.Variable] token against the live GVs", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			// Ensure GVs are loaded for this world.
			if (!TestNotNull(TEXT("global variables"), UArticyGlobalVariables::GetDefault(World)))
				return;

			const FText Format = FText::FromString(TEXT("[GameState.awake]"));
			const FText Result = UArticyTextExtension::Get()->Resolve(World, &Format);

			// The token must have been replaced with the variable's (localized) value.
			TestFalse(TEXT("token was replaced"), Result.ToString().Contains(TEXT("[")));
			TestFalse(TEXT("result not empty"), Result.ToString().IsEmpty());
		});

		It("resolves an [Object.Property] token to the object's property value", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			// Prime the persistent database clone (GetObjectProperty resolves through it).
			if (!TestNotNull(TEXT("database"), UArticyDatabase::Get(World)))
				return;

			// Chr_Hamster has ZIndex 4.0 in the demo; the token resolves it via the object property path.
			const FText Format = FText::FromString(TEXT("[Chr_Hamster.ZIndex]"));
			const FString Res = UArticyTextExtension::Get()->Resolve(World, &Format).ToString();

			// On lookup failure the resolver returns the raw source name; a real value differs from it.
			TestNotEqual(TEXT("resolved, not the raw fallback"), Res, FString(TEXT("Chr_Hamster.ZIndex")));
			TestTrue(TEXT("looks like the z-index value"), Res.Contains(TEXT("4")));
		});

		// NOTE: a [$Type.Type.Property] token test is intentionally absent; that feature is
		// non-functional because UArticyTypeSystem::Types is never populated at runtime.
	});

	Describe("Flow player", [this]()
	{
		It("sets a dialogue start node and explores its branches", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* StartNode = DB->GetObjectByName(FName(TEXT("Dlg_TheTherapist")));
			if (!TestNotNull(TEXT("start dialogue"), StartNode))
				return;

			// Host the flow player component on a throwaway actor in the world.
			AActor* Actor = World->SpawnActor<AActor>();
			if (!TestNotNull(TEXT("actor"), Actor))
				return;

			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(Actor);
			Player->RegisterComponent();

			// Synchronously sets the cursor and explores to the next pause nodes
			// (default PauseOn = DialogueFragment).
			Player->SetStartNodeById(StartNode->GetId());

			TestNotNull(TEXT("cursor set"), Player->GetCursor().GetObject());
			TestTrue(TEXT("explored to branches"), Player->GetAvailableBranches().Num() > 0);

			Actor->Destroy();
		});

		It("advances the cursor by playing a branch", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* StartNode = DB->GetObjectByName(FName(TEXT("Dlg_TheTherapist")));
			if (!TestNotNull(TEXT("start dialogue"), StartNode))
				return;

			AActor* Actor = World->SpawnActor<AActor>();
			if (!TestNotNull(TEXT("actor"), Actor))
				return;

			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(Actor);
			Player->RegisterComponent();
			Player->SetStartNodeById(StartNode->GetId());

			if (!TestTrue(TEXT("has a branch to play"), Player->GetAvailableBranches().Num() > 0))
			{
				Actor->Destroy();
				return;
			}

			const UObject* CursorBefore = Player->GetCursor().GetObject();

			// Play() enqueues the branch; OnTick drains the queue, executing the
			// branch's node scripts and advancing the cursor to the branch target.
			Player->Play(0);
			Player->OnTick(0.0f);

			const UObject* CursorAfter = Player->GetCursor().GetObject();
			TestNotNull(TEXT("cursor after play"), CursorAfter);
			TestTrue(TEXT("cursor advanced"), CursorAfter != CursorBefore);

			Actor->Destroy();
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
