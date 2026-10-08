//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyObject.h"
#include "ArticyEntity.h"
#include "ArticyHelpers.h"
#include "ArticyTextExtension.h"
#include "ArticyTypeSystem.h"
#include "ArticyFlowPlayer.h"
#include "ArticyHierarchyManager.h"
#include "ArticyPluginSettings.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
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
	const TCHAR* DemoTextObject = TEXT("FFr_Cell");
	const TCHAR* DemoTextPart = TEXT("padded cell");
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

		It("resolves $Self against the object a text is resolved for", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Entity = DB->GetObjectByName(FName(DemoEntity));
			if (!Entity)
			{
				AddWarning(MissingContentMessage(DemoEntity));
				return;
			}

			// $Self is the context object; its base property must match what the object reports.
			const FString ViaSelf = UArticyTextExtension::Get()->Resolve(Entity, FString::Printf(TEXT("[$Self.%s]"), DemoEntityProperty)).ToString();
			const FString ViaName = UArticyTextExtension::Get()->Resolve(Entity, FString::Printf(TEXT("[%s.%s]"), DemoEntity, DemoEntityProperty)).ToString();
			TestEqual(TEXT("$Self reads the same property"), ViaSelf, ViaName);
			TestFalse(TEXT("resolved to a value"), ViaSelf.IsEmpty());

			// A bare object token shows the display name, the same one the interface returns.
			const IArticyObjectWithDisplayName* WithDisplayName = Cast<IArticyObjectWithDisplayName>(Entity);
			if (!TestNotNull(TEXT("entity has a display name"), WithDisplayName))
				return;
			TestEqual(TEXT("bare object token is the display name"),
				UArticyTextExtension::Get()->Resolve(Entity, FString(TEXT("[$Self]"))).ToString(),
				WithDisplayName->GetDisplayName().ToString());
			TestEqual(TEXT("technical name lookup matches"),
				UArticyTextExtension::Get()->Resolve(Entity, FString::Printf(TEXT("[%s]"), DemoEntity)).ToString(),
				WithDisplayName->GetDisplayName().ToString());
		});

		It("resolves $Speaker on a dialogue fragment of the demo dialogue", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Dialogue = DB->GetObjectByName(FName(DemoDialogue));
			if (!Dialogue)
			{
				AddWarning(MissingContentMessage(DemoDialogue));
				return;
			}

			// Find a fragment with a speaker among the dialogue's children.
			UArticyObject* Fragment = nullptr;
			for (const TWeakObjectPtr<UArticyObject>& Child : Dialogue->GetChildren())
			{
				const IArticyObjectWithSpeaker* WithSpeaker = Child.IsValid() ? Cast<IArticyObjectWithSpeaker>(Child.Get()) : nullptr;
				if (WithSpeaker && !WithSpeaker->GetSpeakerId().IsNull())
				{
					Fragment = Child.Get();
					break;
				}
			}
			if (!Fragment)
			{
				AddWarning(MissingContentMessage(TEXT("a dialogue fragment with a speaker")));
				return;
			}

			UArticyObject* Speaker = Cast<IArticyObjectWithSpeaker>(Fragment)->GetSpeaker();
			const IArticyObjectWithDisplayName* SpeakerName = Speaker ? Cast<IArticyObjectWithDisplayName>(Speaker) : nullptr;
			if (!TestNotNull(TEXT("speaker with display name"), SpeakerName))
				return;

			TestEqual(TEXT("$Speaker is the fragment's speaker"),
				UArticyTextExtension::Get()->Resolve(Fragment, FString(TEXT("[$Speaker]"))).ToString(),
				SpeakerName->GetDisplayName().ToString());
		});

		It("keeps escaped brackets and drops invalid tokens in generated texts", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			const UArticyPluginSettings* Settings = UArticyPluginSettings::Get();
			if (!TestNotNull(TEXT("settings"), Settings) || !Settings->bAllowInvalidTokens)
			{
				AddWarning(TEXT("Allow invalid tokens is off in this project; skipping."));
				return;
			}

			TestEqual(TEXT("escape"), UArticyTextExtension::Get()->Resolve(World, FString(TEXT("a \\[b\\] c"))).ToString(), FString(TEXT("a [b] c")));
			TestEqual(TEXT("invalid token dropped"), UArticyTextExtension::Get()->Resolve(World, FString(TEXT("a [No.Such.Thing] c"))).ToString(), FString(TEXT("a  c")));
		});

		It("reads $Type information from the generated type system", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyTypeSystem* TypeSystem = UArticyTypeSystem::Get();
			if (!TestNotNull(TEXT("type system"), TypeSystem))
				return;
			if (TypeSystem->Types.Num() == 0)
			{
				AddWarning(TEXT("The type system has no types - has the project been imported since the type system asset was added?"));
				return;
			}

			// Any type with a technical name must report it through $Type.
			FString TypeName;
			for (const auto& Pair : TypeSystem->Types)
			{
				if (!Pair.Value.TechnicalName.IsEmpty())
				{
					TypeName = Pair.Key;
					break;
				}
			}
			if (!TestFalse(TEXT("a type with a technical name exists"), TypeName.IsEmpty()))
				return;

			const FString Token = FString::Printf(TEXT("[$Type.%s.TechnicalName]"), *TypeName);
			TestEqual(TEXT("$Type technical name"), UArticyTextExtension::Get()->Resolve(World, Token).ToString(),
				TypeSystem->Types[TypeName].TechnicalName);
		});
	});

	Describe("Hierarchy", [this]()
	{
		It("exposes the imported project tree", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB) || !TestNotNull(TEXT("project hierarchy"), DB->GetProjectHierarchy()))
				return;

			UArticyHierarchyNode* Root = DB->GetProjectHierarchy()->GetProjectNode();
			if (!TestNotNull(TEXT("project node - has the project been reimported since hierarchy support was added?"), Root))
				return;

			TestEqual(TEXT("root type"), Root->GetArticyType(), FString(TEXT("Project")));
			TestTrue(TEXT("root has no parent"), Root->GetParent().IsNull());
			TestTrue(TEXT("root has children"), Root->GetChildren().Num() > 0);
		});

		It("resolves a known object and walks up to the project node", [this]()
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

			UArticyHierarchyManager* Hierarchy = DB->GetProjectHierarchy();
			UArticyHierarchyNode* Node = Hierarchy ? Hierarchy->GetHierarchyInfo(Lobby->GetId()) : nullptr;
			if (!TestNotNull(TEXT("hierarchy node for the object"), Node))
				return;

			TestEqual(TEXT("technical name"), Node->GetTechnicalName(), FString(DemoFlowFragment));
			TestEqual(TEXT("type"), Node->GetArticyType(), FString(TEXT("FlowFragment")));
			TestTrue(TEXT("node resolves to the database object"), Node->GetObject() == Lobby);
			TestTrue(TEXT("hierarchy parent matches object parent"), Node->GetParent() == Lobby->GetParentID());

			// Every node's parent chain ends at the project node.
			int32 Depth = 0;
			UArticyHierarchyNode* Current = Node;
			while (Current && !Current->GetParent().IsNull() && Depth < 1000)
			{
				Current = Hierarchy->GetHierarchyInfo(Current->GetParent());
				++Depth;
			}
			TestTrue(TEXT("reached the project node"), Current == Hierarchy->GetProjectNode());
		});
	});

	Describe("Type system", [this]()
	{
		It("populates the type map from the imported project", [this]()
		{
			UArticyTypeSystem* TypeSystem = UArticyTypeSystem::Get();
			if (!TestNotNull(TEXT("type system"), TypeSystem))
				return;

			TestTrue(TEXT("types imported - has the project been reimported since upgrading?"),
				TypeSystem->Types.Num() > 0);
		});

		It("gives an imported object a non-empty type", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Entity = DB->GetObjectByName(FName(DemoEntity));
			if (!Entity)
			{
				AddWarning(MissingContentMessage(DemoEntity));
				return;
			}

			const FArticyType Type = Entity->GetArticyType();
			TestFalse(TEXT("has a technical name"), Type.TechnicalName.IsEmpty());
			TestTrue(TEXT("has properties"), Type.GetProperties().Num() > 0);

			// The property the object-property test reads must be described by the type.
			TestEqual(TEXT("property is described"), Type.GetProperty(DemoEntityProperty).TechnicalName,
				FString(DemoEntityProperty));
		});
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

	Describe("Object filtering", [this]()
	{
		It("returns every object for an empty filter", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			TestEqual(TEXT("all objects"), DB->FilterObjects(FString()).Num(), DB->GetAllObjects().Num());
		});

		It("finds an entity by a part of its technical name, by its id and by its display name", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Hamster = DB->GetObjectByName(FName(DemoEntity));
			if (!Hamster)
			{
				AddWarning(MissingContentMessage(DemoEntity));
				return;
			}

			TestTrue(TEXT("technical name part"), DB->FilterObjects(FString(DemoEntity).Left(7)).Contains(Hamster));
			TestTrue(TEXT("hex id"), DB->FilterObjects(ArticyHelpers::Uint64ToHex(Hamster->GetId().Get())).Contains(Hamster));

			// The localized display name is what a user would type.
			const FString DisplayName = Cast<IArticyObjectWithDisplayName>(Hamster)->GetDisplayName().ToString();
			TestFalse(TEXT("has a display name"), DisplayName.IsEmpty());
			TestTrue(TEXT("display name"), DB->FilterObjects(DisplayName).Contains(Hamster));
		});

		It("finds a flow fragment by a part of its text", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Cell = DB->GetObjectByName(FName(DemoTextObject));
			if (!Cell)
			{
				AddWarning(MissingContentMessage(DemoTextObject));
				return;
			}

			TestTrue(TEXT("text part"), DB->FilterObjects(DemoTextPart).Contains(Cell));
		});

		It("narrows a class query with FilterObjectsBasedOn", [this]()
		{
			UWorld* World = GetIntegrationWorld();
			if (!TestNotNull(TEXT("editor world"), World))
				return;

			UArticyDatabase* DB = UArticyDatabase::Get(World);
			if (!TestNotNull(TEXT("database"), DB))
				return;

			UArticyObject* Hamster = DB->GetObjectByName(FName(DemoEntity));
			if (!Hamster)
			{
				AddWarning(MissingContentMessage(DemoEntity));
				return;
			}

			const TArray<UArticyObject*> Entities = DB->GetObjectsOfClass(UArticyEntity::StaticClass());
			const TArray<UArticyObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Entities, DemoEntity);

			TestTrue(TEXT("found"), Found.Contains(Hamster));
			TestTrue(TEXT("narrower than the class query"), Found.Num() < Entities.Num());
			for (const UArticyObject* Object : Found)
			{
				TestTrue(TEXT("still an entity"), Object->IsA(UArticyEntity::StaticClass()));
			}
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
