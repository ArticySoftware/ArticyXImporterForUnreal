//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArticyBaseTypes.h"
#include "ArticyHierarchyNode.h"
#include "ArticyHierarchyManager.generated.h"

/**
 * Gives basic information about all articy objects in your project, exported or not, and their parent-child relationships.
 *
 * This manages information about all objects in the articy project, even objects that were not marked
 * for export, are not currently loaded in a package, or were never exported. This can be helpful to
 * rebuild the exact same project tree found in articy.
 *
 * Access it via UArticyDatabase::GetProjectHierarchy.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyHierarchyManager : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Gets the root hierarchy node, which is also the project node.
	 * @return The project node, or nullptr if no hierarchy was imported.
	 */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	UArticyHierarchyNode* GetProjectNode() const;

	/**
	 * Gets the hierarchy node of the object with the given id.
	 * @param Id The id of the articy object.
	 * @return The found node, or nullptr if the id is not part of the hierarchy.
	 */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	UArticyHierarchyNode* GetHierarchyInfo(FArticyId Id) const;

	/**
	 * Replaces the hierarchy with the given flat, pre-order node list.
	 * Used internally by the ArticyImporter.
	 */
	void SetSerializedNodes(TArray<FArticyHierarchyNodeData> Nodes);

	/** Gets the flat, pre-order node list the hierarchy is built from. */
	const TArray<FArticyHierarchyNodeData>& GetSerializedNodes() const { return SerializedNodes; }

	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;

private:

	/** The persisted hierarchy, as a flat pre-order list. The node tree is built from this on demand. */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	TArray<FArticyHierarchyNodeData> SerializedNodes;

	UPROPERTY(Transient)
	mutable UArticyHierarchyNode* Root = nullptr;

	UPROPERTY(Transient)
	mutable TMap<FArticyId, UArticyHierarchyNode*> HierarchyIdMap;

	mutable bool bIsBuilt = false;

	/** Builds the node tree and id map from SerializedNodes, if not built yet. */
	void EnsureBuilt() const;
	void ResetBuiltNodes() const;
	UArticyHierarchyNode* ReadNodeFromSerializedNodes(int32& Index) const;
};
