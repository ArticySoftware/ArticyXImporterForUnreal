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

// Integration tests: these need a host project that has already imported articy content
// (a generated database + global variables). They are written against the objects and
// variables named below, which come from the ManiacManfred demo project - a project built
// on different articy content will not have them, so each test says what it needs and is
// skipped with a warning when that is missing. The code only touches the plugin's
// base-class API, so the plugin itself never depends on the generated game module.
namespace
{
	// The demo content these tests are written against.
	const TCHAR* DemoFlowFragment = TEXT("FFr_Lobby");
	const TCHAR* DemoDialogue = TEXT("Dlg_TheTherapist");
	const TCHAR* DemoEntity = TEXT("Chr_Hamster");
	const TCHAR* DemoEntityProperty = TEXT("ZIndex");
	const TCHAR* DemoVarNamespace = TEXT("GameState");
	const TCHAR* DemoVarName = TEXT("awake");

	UWorld* GetIntegrationWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	FString MissingContentMessage(const FString& What)
	{
		return FString::Printf(TEXT("Skipped: '%s' is not in this project's imported articy content."), *What);
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

			// The precondition for everything below: a project with nothing imported fails
			// here rather than reporting a skip for each individual piece of demo content.
			const TArray<UArticyObject*> Objects = DB->GetObjectsOfClass(UArticyObject::StaticClass());
			TestTrue(TEXT("database has objects - has articy content been imported?"), Objects.Num() > 0);
		});

		It("finds a known object by its technical name", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Lobby = DB->GetObjectByName(FName(DemoFlowFragment));
			if (!Lobby)
			{
				AddWarning(MissingContentMessage(DemoFlowFragment));
				return;
			}

			TestEqual(TEXT("technical name matches"), Lobby->GetTechnicalName().ToString(), FString(DemoFlowFragment));
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

			// An unknown name resolves to bSucceeded == false rather than a value, so this
			// reports missing content instead of asserting on whatever the default was.
			bool bSucceeded = false;
			GV->GetBoolVariable(FArticyGvName(FName(DemoVarNamespace), FName(DemoVarName)), bSucceeded);
			if (!bSucceeded)
			{
				AddWarning(MissingContentMessage(FString::Printf(TEXT("%s.%s"), DemoVarNamespace, DemoVarName)));
				return;
			}

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

			const FArticyGvName Awake{ FName(DemoVarNamespace), FName(DemoVarName) };
			bool bOk = false;
			const bool bOriginal = GV->GetBoolVariable(Awake, bOk);
			if (!bOk)
			{
				AddWarning(MissingContentMessage(FString::Printf(TEXT("%s.%s"), DemoVarNamespace, DemoVarName)));
				return;
			}

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
			UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
			if (!TestNotNull(TEXT("global variables"), GV))
				return;

			bool bSucceeded = false;
			GV->GetBoolVariable(FArticyGvName(FName(DemoVarNamespace), FName(DemoVarName)), bSucceeded);
			if (!bSucceeded)
			{
				AddWarning(MissingContentMessage(FString::Printf(TEXT("%s.%s"), DemoVarNamespace, DemoVarName)));
				return;
			}

			const FText Format = FText::FromString(FString::Printf(TEXT("[%s.%s]"), DemoVarNamespace, DemoVarName));
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
			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			if (!DB->GetObjectByName(FName(DemoEntity)))
			{
				AddWarning(MissingContentMessage(DemoEntity));
				return;
			}

			// Chr_Hamster has ZIndex 4.0 in the demo; the token resolves it via the object property path.
			const FString Token = FString::Printf(TEXT("%s.%s"), DemoEntity, DemoEntityProperty);
			const FText Format = FText::FromString(FString::Printf(TEXT("[%s]"), *Token));
			const FString Res = UArticyTextExtension::Get()->Resolve(World, &Format).ToString();

			// On lookup failure the resolver returns the raw source name; a real value differs from it.
			TestNotEqual(TEXT("resolved, not the raw fallback"), Res, Token);
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

			UArticyObject* StartNode = DB->GetObjectByName(FName(DemoDialogue));
			if (!StartNode)
			{
				AddWarning(MissingContentMessage(DemoDialogue));
				return;
			}

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

			UArticyObject* StartNode = DB->GetObjectByName(FName(DemoDialogue));
			if (!StartNode)
			{
				AddWarning(MissingContentMessage(DemoDialogue));
				return;
			}

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
