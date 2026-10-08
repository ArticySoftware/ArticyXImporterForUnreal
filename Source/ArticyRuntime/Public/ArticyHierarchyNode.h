//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArticyBaseTypes.h"
#include "ArticyObject.h"
#include "ArticyHierarchyNode.generated.h"

/**
 * Serialized form of a single hierarchy node.
 *
 * The hierarchy is stored as a flat, pre-order list of these entries: each node is followed
 * by its ChildCount children (and, recursively, their descendants).
 * Used internally by the ArticyImporter to persist the project hierarchy.
 */
USTRUCT()
struct ARTICYRUNTIME_API FArticyHierarchyNodeData
{
	GENERATED_BODY()

	/** Id of the articy object this node describes. */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FArticyId Id;

	/** Id of the parent node's object; null for the project (root) node. */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FArticyId Parent;

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FString TechnicalName;

	/** The articy type of the object, e.g. "FlowFragment", "Entity" or "UserFolder". */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FString ArticyType;

	/** Number of direct children following this entry in the pre-order list. */
	UPROPERTY(VisibleAnywhere, Category = "Articy")
	int32 ChildCount = 0;
};

/**
 * Lightweight information about one object of the articy project and its place in the project tree.
 *
 * The hierarchy contains every object of the articy project, including objects that were
 * not exported or whose package is not loaded, so it mirrors the exact project tree found in articy.
 * Nodes are owned by a UArticyHierarchyManager, see UArticyDatabase::GetProjectHierarchy.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyHierarchyNode : public UObject
{
	GENERATED_BODY()

	friend class UArticyHierarchyManager;

public:

	/** Gets the id of the articy object this node describes. */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	FArticyId GetId() const { return Id; }

	/** Gets the id of the parent node's object; null for the project (root) node. */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	FArticyId GetParent() const { return Parent; }

	/** Gets the technical name of the object; may be empty for system objects such as settings. */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	const FString& GetTechnicalName() const { return TechnicalName; }

	/** Gets the articy type of the object, e.g. "FlowFragment", "Entity" or "UserFolder". */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	const FString& GetArticyType() const { return ArticyType; }

	/** Gets the child nodes, in the order they appear in articy. */
	UFUNCTION(BlueprintPure, Category = "Articy|Hierarchy")
	const TArray<UArticyHierarchyNode*>& GetChildren() const { return Children; }

	/**
	 * Tries to return the underlying object, if it is part of the export and currently loaded.
	 * The object is resolved through the database that owns this node's hierarchy.
	 * @param CloneId The clone ID of the object.
	 * @param CastTo The class to cast the object to.
	 * @return The object, or nullptr if it was not exported, its package is not loaded, or it is not of class CastTo.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy|Hierarchy", meta = (DeterminesOutputType = "CastTo", AdvancedDisplay = "CloneId"))
	UArticyObject* GetObject(int32 CloneId = 0, TSubclassOf<UArticyObject> CastTo = nullptr) const;

	template<typename T>
	T* GetObject(int32 CloneId = 0) const { return Cast<T>(GetObject(CloneId)); }

protected:

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FArticyId Id;

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FArticyId Parent;

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FString TechnicalName;

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FString ArticyType;

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	TArray<UArticyHierarchyNode*> Children;
};
