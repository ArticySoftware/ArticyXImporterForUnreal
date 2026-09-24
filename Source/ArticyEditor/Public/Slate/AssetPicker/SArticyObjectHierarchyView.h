//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyBaseTypes.h"
#include "ArticyHierarchyManager.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UArticyObject;

/**
 * @brief An entry of the object picker's hierarchy view: one node of the articy project tree.
 */
struct ARTICYEDITOR_API FArticyHierarchyTreeItem
{
	/** Id of the articy object; null for the group of objects that are missing from the hierarchy. */
	FArticyId Id;
	FString TechnicalName;
	FString ArticyType;

	/** The exported object, if there is one. */
	TWeakObjectPtr<UArticyObject> Object;

	/** True if the object passes the picker's filters and can be picked. Other items are only shown as the path to one. */
	bool bSelectable = false;

	TWeakPtr<FArticyHierarchyTreeItem> Parent;
	TArray<TSharedPtr<FArticyHierarchyTreeItem>> Children;

	/** The label shown for the item: the object's display name, falling back to its technical name and type. */
	FString GetLabel() const;

	/**
	 * @brief Builds the visible tree below the project node.
	 *
	 * Only branches containing a selectable object are kept, so the result shows the path through the
	 * project tree to each selectable object. Selectable objects that are not part of the hierarchy are
	 * collected in a trailing group item.
	 *
	 * @param Hierarchy The project hierarchy; may be null or empty.
	 * @param SelectableObjects The objects that can be picked, by id.
	 * @param ResolveObject Optional; resolves the objects of non-selectable items that are kept, for their labels and icons.
	 * @return The top-level items (the children of the project node).
	 */
	static TArray<TSharedPtr<FArticyHierarchyTreeItem>> BuildTree(
		const UArticyHierarchyManager* Hierarchy,
		const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects,
		TFunction<UArticyObject*(const FArticyId&)> ResolveObject = nullptr);

private:
	static TSharedPtr<FArticyHierarchyTreeItem> BuildItem(
		const UArticyHierarchyNode* Node,
		const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects,
		const TFunction<UArticyObject*(const FArticyId&)>& ResolveObject,
		TSet<FArticyId>& OutPlacedIds);
};

using FArticyHierarchyTreeItemPtr = TSharedPtr<FArticyHierarchyTreeItem>;

DECLARE_DELEGATE_OneParam(FOnArticyHierarchyObjectSelected, UArticyObject*);

/**
 * @brief A tree view of the articy project hierarchy, similar to articy's navigator, used by the object picker.
 */
class ARTICYEDITOR_API SArticyObjectHierarchyView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArticyObjectHierarchyView) {}
		/** Called when the user picks a selectable object. */
		SLATE_EVENT(FOnArticyHierarchyObjectSelected, OnObjectSelected)
		/** Text to highlight in the item labels, usually the search text. */
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * @brief Rebuilds the tree from the project hierarchy of the database asset.
	 *
	 * @param SelectableObjects The objects that pass the picker's filters, by id.
	 * @param bExpandAll Expand every item, e.g. while searching, instead of restoring the user's expansion state.
	 */
	void SetSelectableObjects(const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects, bool bExpandAll);

	/**
	 * @brief Expands the path to an object, highlights it and scrolls it into view, without picking it.
	 *
	 * @param Id The id of the object to reveal.
	 */
	void RevealObject(const FArticyId& Id);

private:
	TSharedRef<ITableRow> OnGenerateRow(FArticyHierarchyTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnGetChildren(FArticyHierarchyTreeItemPtr Item, TArray<FArticyHierarchyTreeItemPtr>& OutChildren) const;
	void OnSelectionChanged(FArticyHierarchyTreeItemPtr Item, ESelectInfo::Type SelectInfo);
	void OnExpansionChanged(FArticyHierarchyTreeItemPtr Item, bool bExpanded);

	/** Applies the remembered expansion state, or expands everything, to the current items. */
	void ApplyExpansion(const TArray<FArticyHierarchyTreeItemPtr>& Items, bool bExpandAll);
	FArticyHierarchyTreeItemPtr FindItem(const TArray<FArticyHierarchyTreeItemPtr>& Items, const FArticyId& Id) const;

	FText GetEmptyText() const;
	EVisibility GetEmptyTextVisibility() const;

	const FSlateBrush* GetItemIcon(const FArticyHierarchyTreeItem& Item) const;

private:
	FOnArticyHierarchyObjectSelected OnObjectSelected;
	TAttribute<FText> HighlightText;

	TSharedPtr<STreeView<FArticyHierarchyTreeItemPtr>> TreeView;
	TArray<FArticyHierarchyTreeItemPtr> RootItems;

	/** Hierarchy built from the import data, for databases generated before the hierarchy was stored with them. */
	TStrongObjectPtr<UArticyHierarchyManager> FallbackHierarchy;

	/** Whether the database has hierarchy data at all, to tell an empty hierarchy apart from an empty filter result. */
	bool bHasHierarchy = false;
	/** Set while expanding items programmatically, so it isn't recorded as the user's expansion state. */
	bool bIsApplyingExpansion = false;

	/** The items the user expanded, kept for the editor session so reopened pickers look the same. */
	static TSet<FArticyId> ExpandedIds;
	/** Whether ExpandedIds has been initialized with the default expansion (the top-level items). */
	static bool bExpansionInitialized;
};
