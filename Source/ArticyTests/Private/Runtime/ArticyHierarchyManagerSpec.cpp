//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyHierarchyManager.h"
#include "ArticyDatabase.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyHierarchyNodeData MakeNode(uint64 Id, uint64 Parent, const FString& TechnicalName, const FString& Type, int32 ChildCount)
	{
		FArticyHierarchyNodeData Node;
		Node.Id = Id;
		Node.Parent = Parent;
		Node.TechnicalName = TechnicalName;
		Node.ArticyType = Type;
		Node.ChildCount = ChildCount;
		return Node;
	}

	// Project
	// +- Flow
	// |  +- FFr_A
	// |  |  +- DFr_1
	// |  +- FFr_B
	// +- Entities
	TArray<FArticyHierarchyNodeData> MakeSampleNodes()
	{
		return {
			MakeNode(0x100, 0, TEXT("Project"), TEXT("Project"), 2),
			MakeNode(0x200, 0x100, TEXT("Flow"), TEXT("Flow"), 2),
			MakeNode(0x210, 0x200, TEXT("FFr_A"), TEXT("FlowFragment"), 1),
			MakeNode(0x211, 0x210, TEXT("DFr_1"), TEXT("DialogueFragment"), 0),
			MakeNode(0x220, 0x200, TEXT("FFr_B"), TEXT("FlowFragment"), 0),
			MakeNode(0x300, 0x100, TEXT("Entities"), TEXT("Entities"), 0),
		};
	}

	UArticyHierarchyManager* MakeManager(TArray<FArticyHierarchyNodeData> Nodes)
	{
		UArticyHierarchyManager* Manager = NewObject<UArticyHierarchyManager>();
		Manager->SetSerializedNodes(MoveTemp(Nodes));
		return Manager;
	}
}

BEGIN_DEFINE_SPEC(FArticyHierarchyManagerSpec, "Articy.Runtime.Hierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyHierarchyManagerSpec)

void FArticyHierarchyManagerSpec::Define()
{
	Describe("GetProjectNode", [this]()
	{
		It("is null when no hierarchy was imported", [this]()
		{
			UArticyHierarchyManager* Manager = NewObject<UArticyHierarchyManager>();
			TestNull(TEXT("project node"), Manager->GetProjectNode());
			TestNull(TEXT("lookup"), Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x100))));
		});

		It("returns the root with its data and a null parent", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			UArticyHierarchyNode* Root = Manager->GetProjectNode();
			if (!TestNotNull(TEXT("project node"), Root))
				return;

			TestEqual(TEXT("id"), Root->GetId().Get(), static_cast<uint64>(0x100));
			TestTrue(TEXT("parent is null"), Root->GetParent().IsNull());
			TestEqual(TEXT("technical name"), Root->GetTechnicalName(), FString(TEXT("Project")));
			TestEqual(TEXT("type"), Root->GetArticyType(), FString(TEXT("Project")));
		});

		It("rebuilds the tree in pre-order, keeping child order", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			UArticyHierarchyNode* Root = Manager->GetProjectNode();
			if (!TestNotNull(TEXT("project node"), Root) || !TestEqual(TEXT("root children"), Root->GetChildren().Num(), 2))
				return;

			UArticyHierarchyNode* Flow = Root->GetChildren()[0];
			TestEqual(TEXT("first child"), Flow->GetTechnicalName(), FString(TEXT("Flow")));
			TestEqual(TEXT("second child"), Root->GetChildren()[1]->GetTechnicalName(), FString(TEXT("Entities")));

			if (!TestEqual(TEXT("flow children"), Flow->GetChildren().Num(), 2))
				return;
			UArticyHierarchyNode* FragmentA = Flow->GetChildren()[0];
			TestEqual(TEXT("FFr_A"), FragmentA->GetTechnicalName(), FString(TEXT("FFr_A")));
			TestEqual(TEXT("FFr_B"), Flow->GetChildren()[1]->GetTechnicalName(), FString(TEXT("FFr_B")));
			if (TestEqual(TEXT("FFr_A children"), FragmentA->GetChildren().Num(), 1))
			{
				TestEqual(TEXT("DFr_1"), FragmentA->GetChildren()[0]->GetTechnicalName(), FString(TEXT("DFr_1")));
			}
			TestEqual(TEXT("leaf has no children"), Root->GetChildren()[1]->GetChildren().Num(), 0);
		});
	});

	Describe("GetHierarchyInfo", [this]()
	{
		It("finds every node by id, including the root", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			for (const FArticyHierarchyNodeData& Data : MakeSampleNodes())
			{
				UArticyHierarchyNode* Node = Manager->GetHierarchyInfo(Data.Id);
				if (TestNotNull(*FString::Printf(TEXT("node %s"), *Data.TechnicalName), Node))
				{
					TestEqual(TEXT("technical name"), Node->GetTechnicalName(), Data.TechnicalName);
				}
			}
		});

		It("returns the node that is part of the tree", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			UArticyHierarchyNode* Root = Manager->GetProjectNode();
			TestTrue(TEXT("same object as in tree"), Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x200))) == Root->GetChildren()[0]);
		});

		It("returns null for an unknown id", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			TestNull(TEXT("unknown id"), Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0xDEAD))));
		});

		It("walks up to the root through the parent ids", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			UArticyHierarchyNode* Node = Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x211)));
			TArray<FString> Path;
			while (Node)
			{
				Path.Insert(Node->GetTechnicalName(), 0);
				Node = Node->GetParent().IsNull() ? nullptr : Manager->GetHierarchyInfo(Node->GetParent());
			}
			TestEqual(TEXT("path"), FString::Join(Path, TEXT("/")), FString(TEXT("Project/Flow/FFr_A/DFr_1")));
		});

		It("keeps the first node when an id occurs twice", [this]()
		{
			TArray<FArticyHierarchyNodeData> Nodes = {
				MakeNode(0x100, 0, TEXT("Project"), TEXT("Project"), 2),
				MakeNode(0x200, 0x100, TEXT("First"), TEXT("UserFolder"), 0),
				MakeNode(0x200, 0x100, TEXT("Second"), TEXT("UserFolder"), 0),
			};
			AddExpectedError(TEXT("more than once"), EAutomationExpectedErrorFlags::Contains, 1);
			UArticyHierarchyManager* Manager = MakeManager(Nodes);
			UArticyHierarchyNode* Node = Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x200)));
			if (TestNotNull(TEXT("node"), Node))
			{
				TestEqual(TEXT("first wins"), Node->GetTechnicalName(), FString(TEXT("First")));
			}
			TestEqual(TEXT("both stay in the tree"), Manager->GetProjectNode()->GetChildren().Num(), 2);
		});

		It("does not read past the list when child counts are inconsistent", [this]()
		{
			TArray<FArticyHierarchyNodeData> Nodes = {
				MakeNode(0x100, 0, TEXT("Project"), TEXT("Project"), 5),
				MakeNode(0x200, 0x100, TEXT("Only"), TEXT("UserFolder"), 0),
			};
			UArticyHierarchyManager* Manager = MakeManager(Nodes);
			UArticyHierarchyNode* Root = Manager->GetProjectNode();
			if (TestNotNull(TEXT("project node"), Root))
			{
				TestEqual(TEXT("children present"), Root->GetChildren().Num(), 1);
			}
		});
	});

	Describe("SetSerializedNodes", [this]()
	{
		It("replaces a previously built hierarchy", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			TestNotNull(TEXT("old node"), Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x211))));

			Manager->SetSerializedNodes({ MakeNode(0x900, 0, TEXT("Other"), TEXT("Project"), 0) });
			TestNull(TEXT("old node gone"), Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x211))));
			if (TestNotNull(TEXT("new root"), Manager->GetProjectNode()))
			{
				TestEqual(TEXT("new root name"), Manager->GetProjectNode()->GetTechnicalName(), FString(TEXT("Other")));
			}
		});
	});

	Describe("Database integration", [this]()
	{
		It("exposes a hierarchy manager on every database", [this]()
		{
			UArticyDatabase* Database = NewObject<UArticyDatabase>();
			TestNotNull(TEXT("project hierarchy"), Database->GetProjectHierarchy());
		});

		It("gives a duplicated database its own nodes that resolve through the copy", [this]()
		{
			UArticyDatabase* Original = NewObject<UArticyDatabase>();
			Original->GetProjectHierarchy()->SetSerializedNodes(MakeSampleNodes());
			UArticyHierarchyNode* OriginalRoot = Original->GetProjectHierarchy()->GetProjectNode();

			UArticyDatabase* Copy = DuplicateObject(Original, GetTransientPackage());
			if (!TestNotNull(TEXT("copy"), Copy) || !TestNotNull(TEXT("copy hierarchy"), Copy->GetProjectHierarchy()))
				return;

			TestTrue(TEXT("hierarchy manager is duplicated"), Copy->GetProjectHierarchy() != Original->GetProjectHierarchy());
			UArticyHierarchyNode* CopyRoot = Copy->GetProjectHierarchy()->GetProjectNode();
			if (!TestNotNull(TEXT("copy root"), CopyRoot))
				return;

			TestTrue(TEXT("nodes are not shared"), CopyRoot != OriginalRoot);
			TestTrue(TEXT("nodes belong to the copy"), CopyRoot->GetTypedOuter<UArticyDatabase>() == Copy);
			TestEqual(TEXT("same data"), CopyRoot->GetTechnicalName(), OriginalRoot->GetTechnicalName());
			TestNotNull(TEXT("lookup in copy"), Copy->GetProjectHierarchy()->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x211))));
		});

		It("returns no object for nodes whose object is not loaded", [this]()
		{
			UArticyDatabase* Database = NewObject<UArticyDatabase>();
			Database->GetProjectHierarchy()->SetSerializedNodes(MakeSampleNodes());
			UArticyHierarchyNode* Node = Database->GetProjectHierarchy()->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x210)));
			if (TestNotNull(TEXT("node"), Node))
			{
				TestNull(TEXT("object"), Node->GetObject());
			}
		});

		It("returns no object for a node outside a database", [this]()
		{
			UArticyHierarchyManager* Manager = MakeManager(MakeSampleNodes());
			TestNull(TEXT("object"), Manager->GetProjectNode()->GetObject());
		});
	});
}

#endif
