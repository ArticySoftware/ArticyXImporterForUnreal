//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyHierarchyManager.h"
#include "ArticyRuntimeModule.h"

/**
 * Gets the root hierarchy node, which is also the project node.
 * @return The project node, or nullptr if no hierarchy was imported.
 */
UArticyHierarchyNode* UArticyHierarchyManager::GetProjectNode() const
{
	EnsureBuilt();
	return Root;
}

/**
 * Gets the hierarchy node of the object with the given id.
 * @param Id The id of the articy object.
 * @return The found node, or nullptr if the id is not part of the hierarchy.
 */
UArticyHierarchyNode* UArticyHierarchyManager::GetHierarchyInfo(FArticyId Id) const
{
	EnsureBuilt();
	UArticyHierarchyNode* const* Node = HierarchyIdMap.Find(Id);
	return Node ? *Node : nullptr;
}

/**
 * Replaces the hierarchy with the given flat, pre-order node list.
 * @param Nodes The nodes, each followed by its ChildCount children.
 */
void UArticyHierarchyManager::SetSerializedNodes(TArray<FArticyHierarchyNodeData> Nodes)
{
	SerializedNodes = MoveTemp(Nodes);
	ResetBuiltNodes();
}

void UArticyHierarchyManager::PostLoad()
{
	Super::PostLoad();
	ResetBuiltNodes();
}

void UArticyHierarchyManager::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	// Nodes are outered to their manager so they resolve objects through its database;
	// rebuild them for the copy rather than sharing the original's.
	ResetBuiltNodes();
}

void UArticyHierarchyManager::ResetBuiltNodes() const
{
	Root = nullptr;
	HierarchyIdMap.Reset();
	bIsBuilt = false;
}

void UArticyHierarchyManager::EnsureBuilt() const
{
	if (bIsBuilt)
		return;
	bIsBuilt = true;

	if (SerializedNodes.Num() == 0)
		return;

	HierarchyIdMap.Reserve(SerializedNodes.Num());

	int32 Index = 0;
	Root = ReadNodeFromSerializedNodes(Index);

	if (Index != SerializedNodes.Num() - 1)
	{
		UE_LOG(LogArticyRuntime, Warning, TEXT("Articy hierarchy data is inconsistent: read %d of %d nodes. Reimport the articy project to fix it."),
			Index + 1, SerializedNodes.Num());
	}
}

/**
 * Creates the node at Index and, recursively, its children, which follow it in the pre-order list.
 * @param Index The index of the node to read; on return, the index of the last node read.
 * @return The created node.
 */
UArticyHierarchyNode* UArticyHierarchyManager::ReadNodeFromSerializedNodes(int32& Index) const
{
	const FArticyHierarchyNodeData& Data = SerializedNodes[Index];

	UArticyHierarchyNode* Node = NewObject<UArticyHierarchyNode>(const_cast<UArticyHierarchyManager*>(this), NAME_None, RF_Transient);
	Node->Id = Data.Id;
	Node->Parent = Data.Parent;
	Node->TechnicalName = Data.TechnicalName;
	Node->ArticyType = Data.ArticyType;

	if (HierarchyIdMap.Contains(Data.Id))
	{
		UE_LOG(LogArticyRuntime, Warning, TEXT("Articy hierarchy contains id %s more than once; lookups return the first node."),
			*ArticyHelpers::Uint64ToHex(Data.Id.Get()));
	}
	else
	{
		HierarchyIdMap.Add(Data.Id, Node);
	}

	Node->Children.Reserve(Data.ChildCount);
	for (int32 i = 0; i < Data.ChildCount && Index + 1 < SerializedNodes.Num(); ++i)
	{
		++Index;
		Node->Children.Add(ReadNodeFromSerializedNodes(Index));
	}

	return Node;
}
