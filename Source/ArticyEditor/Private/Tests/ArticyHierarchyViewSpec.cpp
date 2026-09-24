//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Slate/AssetPicker/SArticyObjectHierarchyView.h"
#include "ArticyHierarchyManager.h"

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
	// |  |  +- DFr_2
	// |  +- FFr_B
	// +- Entities
	// |  +- Chr_X
	// +- Settings
	UArticyHierarchyManager* MakeHierarchy()
	{
		UArticyHierarchyManager* Manager = NewObject<UArticyHierarchyManager>();
		Manager->SetSerializedNodes({
			MakeNode(0x100, 0, TEXT("Project"), TEXT("Project"), 3),
			MakeNode(0x200, 0x100, TEXT("Flow"), TEXT("Flow"), 2),
			MakeNode(0x210, 0x200, TEXT("FFr_A"), TEXT("FlowFragment"), 2),
			MakeNode(0x211, 0x210, TEXT("DFr_1"), TEXT("DialogueFragment"), 0),
			MakeNode(0x212, 0x210, TEXT("DFr_2"), TEXT("DialogueFragment"), 0),
			MakeNode(0x220, 0x200, TEXT("FFr_B"), TEXT("FlowFragment"), 0),
			MakeNode(0x300, 0x100, TEXT("Entities"), TEXT("Entities"), 1),
			MakeNode(0x310, 0x300, TEXT("Chr_X"), TEXT("Entity"), 0),
			MakeNode(0x400, 0x100, TEXT("Settings"), TEXT("ProjectSettingsFolder"), 0),
		});
		return Manager;
	}

	TMap<FArticyId, TWeakObjectPtr<UArticyObject>> Selectable(std::initializer_list<uint64> Ids)
	{
		TMap<FArticyId, TWeakObjectPtr<UArticyObject>> Result;
		for (uint64 Id : Ids)
		{
			Result.Add(FArticyId(Id), nullptr);
		}
		return Result;
	}

	TArray<FString> Labels(const TArray<TSharedPtr<FArticyHierarchyTreeItem>>& Items)
	{
		TArray<FString> Result;
		for (const TSharedPtr<FArticyHierarchyTreeItem>& Item : Items)
		{
			Result.Add(Item->GetLabel());
		}
		return Result;
	}
}

BEGIN_DEFINE_SPEC(FArticyHierarchyViewSpec, "Articy.Editor.HierarchyView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyHierarchyViewSpec)

void FArticyHierarchyViewSpec::Define()
{
	Describe("FArticyHierarchyTreeItem::BuildTree", [this]()
	{
		It("starts below the project node and keeps articy's order", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x211, 0x310 }));
			TestEqual(TEXT("top level"), FString::Join(Labels(Roots), TEXT(",")), FString(TEXT("Flow,Entities")));
		});

		It("keeps only the branches that lead to a selectable object", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x211 }));
			if (!TestEqual(TEXT("top level"), Roots.Num(), 1))
				return;

			const auto& Flow = Roots[0];
			TestFalse(TEXT("folder is not selectable"), Flow->bSelectable);
			if (!TestEqual(TEXT("flow children"), Flow->Children.Num(), 1))
				return;

			const auto& FragmentA = Flow->Children[0];
			TestEqual(TEXT("path through FFr_A"), FragmentA->GetLabel(), FString(TEXT("FFr_A")));
			TestFalse(TEXT("ancestor is not selectable"), FragmentA->bSelectable);
			if (TestEqual(TEXT("FFr_A children"), FragmentA->Children.Num(), 1))
			{
				TestEqual(TEXT("leaf"), FragmentA->Children[0]->GetLabel(), FString(TEXT("DFr_1")));
				TestTrue(TEXT("leaf is selectable"), FragmentA->Children[0]->bSelectable);
			}
		});

		It("keeps selectable objects that have children", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x210 }));
			if (!TestEqual(TEXT("top level"), Roots.Num(), 1) || !TestEqual(TEXT("flow children"), Roots[0]->Children.Num(), 1))
				return;

			const auto& FragmentA = Roots[0]->Children[0];
			TestTrue(TEXT("selectable"), FragmentA->bSelectable);
			TestEqual(TEXT("unselectable children pruned"), FragmentA->Children.Num(), 0);
		});

		It("links every item to its parent", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x212 }));
			if (!TestEqual(TEXT("top level"), Roots.Num(), 1))
				return;

			const auto& Leaf = Roots[0]->Children[0]->Children[0];
			TestTrue(TEXT("leaf -> FFr_A"), Leaf->Parent.Pin() == Roots[0]->Children[0]);
			TestTrue(TEXT("FFr_A -> Flow"), Roots[0]->Children[0]->Parent.Pin() == Roots[0]);
			TestFalse(TEXT("top level has no parent"), Roots[0]->Parent.IsValid());
		});

		It("is empty when nothing is selectable", [this]()
		{
			TestEqual(TEXT("items"), FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({})).Num(), 0);
		});

		It("groups selectable objects that are missing from the hierarchy", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x211, 0x999 }));
			if (!TestEqual(TEXT("top level"), Roots.Num(), 2))
				return;

			const auto& Group = Roots.Last();
			TestTrue(TEXT("group has no id"), Group->Id.IsNull());
			TestFalse(TEXT("group is not selectable"), Group->bSelectable);
			if (TestEqual(TEXT("group children"), Group->Children.Num(), 1))
			{
				TestEqual(TEXT("missing object"), Group->Children[0]->Id.Get(), static_cast<uint64>(0x999));
				TestTrue(TEXT("missing object is selectable"), Group->Children[0]->bSelectable);
			}
		});

		It("puts every selectable object in the group when there is no hierarchy", [this]()
		{
			const auto Roots = FArticyHierarchyTreeItem::BuildTree(nullptr, Selectable({ 0x211, 0x310 }));
			if (TestEqual(TEXT("top level"), Roots.Num(), 1))
			{
				TestEqual(TEXT("group children"), Roots[0]->Children.Num(), 2);
			}
		});

		It("resolves the objects of kept ancestors only", [this]()
		{
			TSet<uint64> Resolved;
			FArticyHierarchyTreeItem::BuildTree(MakeHierarchy(), Selectable({ 0x211 }), [&Resolved](const FArticyId& Id) -> UArticyObject*
				{
					Resolved.Add(Id.Get());
					return nullptr;
				});

			TestTrue(TEXT("Flow resolved"), Resolved.Contains(0x200));
			TestTrue(TEXT("FFr_A resolved"), Resolved.Contains(0x210));
			TestFalse(TEXT("pruned FFr_B not resolved"), Resolved.Contains(0x220));
			TestFalse(TEXT("pruned Entities not resolved"), Resolved.Contains(0x300));
		});
	});

	Describe("FArticyHierarchyTreeItem::GetLabel", [this]()
	{
		It("falls back from technical name to type", [this]()
		{
			FArticyHierarchyTreeItem Item;
			Item.Id = FArticyId(static_cast<uint64>(0x1));
			Item.ArticyType = TEXT("ProjectSettingsFlow");
			TestEqual(TEXT("type"), Item.GetLabel(), FString(TEXT("ProjectSettingsFlow")));

			Item.TechnicalName = TEXT("Settings");
			TestEqual(TEXT("technical name"), Item.GetLabel(), FString(TEXT("Settings")));
		});
	});
}

#endif
