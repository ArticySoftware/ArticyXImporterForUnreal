//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyImportData.h"
#include "ArticyHierarchyManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// Same shape as the hierarchy.json file of an articy export: a single project node,
	// nested recursively. Leaves omit "Children", as the exporter does.
	const TCHAR* HierarchyJson = TEXT(R"({
		"Id": "0x0100000100000001",
		"TechnicalName": "MyProject",
		"Type": "Project",
		"Children": [
			{
				"Id": "0x03000000F917511B",
				"TechnicalName": "ProjectSettings",
				"Type": "ProjectSettingsFolder",
				"Children": [
					{ "Id": "0x01000001000011BE", "TechnicalName": "", "Type": "ProjectSettingsFlow" }
				]
			},
			{
				"Id": "0x0300000043CE43F7",
				"TechnicalName": "Flow",
				"Type": "Flow",
				"Children": [
					{
						"Id": "0x010000010000029E",
						"TechnicalName": "FFr_Cell",
						"Type": "FlowFragment",
						"Children": [
							{ "Id": "0x01000001000002A0", "TechnicalName": "DFr_Hello", "Type": "DialogueFragment" }
						]
					},
					{ "Id": "0x01000001000002B0", "TechnicalName": "Hub_End", "Type": "Hub", "Children": [] }
				]
			}
		]
	})");

	TSharedPtr<FJsonObject> ParseJson(const TCHAR* Text)
	{
		TSharedPtr<FJsonObject> Json;
		FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Text), Json);
		return Json;
	}

	TArray<FArticyHierarchyNodeData> ImportNodes(const TCHAR* Text)
	{
		UArticyImportData* Data = NewObject<UArticyImportData>();
		FADIHierarchy Hierarchy;
		Hierarchy.ImportFromJson(Data, ParseJson(Text));

		TArray<FArticyHierarchyNodeData> Nodes;
		Hierarchy.BuildRuntimeNodes(Nodes);
		return Nodes;
	}
}

BEGIN_DEFINE_SPEC(FArticyHierarchyImportSpec, "Articy.Editor.Hierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyHierarchyImportSpec)

void FArticyHierarchyImportSpec::Define()
{
	Describe("FADIHierarchy::BuildRuntimeNodes", [this]()
	{
		It("is empty when no hierarchy was imported", [this]()
		{
			FADIHierarchy Hierarchy;
			TArray<FArticyHierarchyNodeData> Nodes;
			Nodes.AddDefaulted();
			Hierarchy.BuildRuntimeNodes(Nodes);
			TestEqual(TEXT("nodes"), Nodes.Num(), 0);
		});

		It("flattens the tree in pre-order with child counts", [this]()
		{
			const TArray<FArticyHierarchyNodeData> Nodes = ImportNodes(HierarchyJson);
			if (!TestEqual(TEXT("node count"), Nodes.Num(), 7))
				return;

			const TArray<FString> ExpectedNames = { TEXT("MyProject"), TEXT("ProjectSettings"), TEXT(""), TEXT("Flow"), TEXT("FFr_Cell"), TEXT("DFr_Hello"), TEXT("Hub_End") };
			const TArray<int32> ExpectedChildCounts = { 2, 1, 0, 2, 1, 0, 0 };
			for (int32 i = 0; i < Nodes.Num(); ++i)
			{
				TestEqual(*FString::Printf(TEXT("name %d"), i), Nodes[i].TechnicalName, ExpectedNames[i]);
				TestEqual(*FString::Printf(TEXT("child count %d"), i), Nodes[i].ChildCount, ExpectedChildCounts[i]);
			}
		});

		It("parses hex ids and types", [this]()
		{
			const TArray<FArticyHierarchyNodeData> Nodes = ImportNodes(HierarchyJson);
			if (!TestEqual(TEXT("node count"), Nodes.Num(), 7))
				return;

			TestEqual(TEXT("root id"), Nodes[0].Id.Get(), static_cast<uint64>(0x0100000100000001ull));
			TestEqual(TEXT("fragment id"), Nodes[4].Id.Get(), static_cast<uint64>(0x010000010000029Eull));
			TestEqual(TEXT("root type"), Nodes[0].ArticyType, FString(TEXT("Project")));
			TestEqual(TEXT("fragment type"), Nodes[4].ArticyType, FString(TEXT("FlowFragment")));
		});

		It("gives the root a null parent and every other node its parent's id", [this]()
		{
			const TArray<FArticyHierarchyNodeData> Nodes = ImportNodes(HierarchyJson);
			if (!TestEqual(TEXT("node count"), Nodes.Num(), 7))
				return;

			TestTrue(TEXT("root parent is null"), Nodes[0].Parent.IsNull());
			TestEqual(TEXT("settings -> project"), Nodes[1].Parent.Get(), Nodes[0].Id.Get());
			TestEqual(TEXT("settings flow -> settings"), Nodes[2].Parent.Get(), Nodes[1].Id.Get());
			TestEqual(TEXT("fragment -> flow"), Nodes[4].Parent.Get(), Nodes[3].Id.Get());
			TestEqual(TEXT("dialogue fragment -> fragment"), Nodes[5].Parent.Get(), Nodes[4].Id.Get());
			TestEqual(TEXT("hub -> flow"), Nodes[6].Parent.Get(), Nodes[3].Id.Get());
		});
	});

	Describe("Round trip into UArticyHierarchyManager", [this]()
	{
		It("rebuilds the imported tree", [this]()
		{
			UArticyHierarchyManager* Manager = NewObject<UArticyHierarchyManager>();
			Manager->SetSerializedNodes(ImportNodes(HierarchyJson));

			UArticyHierarchyNode* Root = Manager->GetProjectNode();
			if (!TestNotNull(TEXT("project node"), Root))
				return;
			TestEqual(TEXT("project name"), Root->GetTechnicalName(), FString(TEXT("MyProject")));
			TestEqual(TEXT("project children"), Root->GetChildren().Num(), 2);

			UArticyHierarchyNode* Dialogue = Manager->GetHierarchyInfo(FArticyId(static_cast<uint64>(0x01000001000002A0ull)));
			if (!TestNotNull(TEXT("dialogue fragment"), Dialogue))
				return;
			UArticyHierarchyNode* Fragment = Manager->GetHierarchyInfo(Dialogue->GetParent());
			if (TestNotNull(TEXT("parent lookup"), Fragment))
			{
				TestEqual(TEXT("parent name"), Fragment->GetTechnicalName(), FString(TEXT("FFr_Cell")));
				TestTrue(TEXT("parent lists child"), Fragment->GetChildren().Contains(Dialogue));
			}
		});
	});
}

#endif
