//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Slate/AssetPicker/SArticyObjectHierarchyView.h"
#include "ArticyDatabase.h"
#include "ArticyEditorStyle.h"
#include "ArticyHierarchyManager.h"
#include "ArticyImportData.h"
#include "ArticyObject.h"
#include "Slate/AssetPicker/SArticyObjectToolTip.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ArticyObjectHierarchyView"

TSet<FArticyId> SArticyObjectHierarchyView::ExpandedIds;
bool SArticyObjectHierarchyView::bExpansionInitialized = false;

//---------------------------------------------------------------------------//

/**
 * @brief Gets the label shown for the item.
 *
 * @return The object's display name, falling back to its technical name and type.
 */
FString FArticyHierarchyTreeItem::GetLabel() const
{
	if (const UArticyObject* ArticyObject = Object.Get())
	{
		const FString DisplayName = UserInterfaceHelperFunctions::GetDisplayName(ArticyObject);
		if (!DisplayName.IsEmpty() && DisplayName != TEXT("None"))
		{
			return DisplayName;
		}
	}

	if (!TechnicalName.IsEmpty())
	{
		return TechnicalName;
	}

	if (!ArticyType.IsEmpty())
	{
		return ArticyType;
	}

	return Id.IsNull() ? LOCTEXT("NotInHierarchy", "Not in project hierarchy").ToString() : ArticyHelpers::Uint64ToHex(Id.Get());
}

/**
 * @brief Builds the visible tree below the project node.
 *
 * @param Hierarchy The project hierarchy; may be null or empty.
 * @param SelectableObjects The objects that can be picked, by id.
 * @param ResolveObject Optional; resolves the objects of non-selectable items that are kept.
 * @return The top-level items (the children of the project node).
 */
TArray<TSharedPtr<FArticyHierarchyTreeItem>> FArticyHierarchyTreeItem::BuildTree(
	const UArticyHierarchyManager* Hierarchy,
	const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects,
	TFunction<UArticyObject*(const FArticyId&)> ResolveObject)
{
	TArray<TSharedPtr<FArticyHierarchyTreeItem>> RootItems;
	TSet<FArticyId> PlacedIds;

	if (const UArticyHierarchyNode* ProjectNode = Hierarchy ? Hierarchy->GetProjectNode() : nullptr)
	{
		// Like articy's navigator, the project node itself is not shown
		for (const UArticyHierarchyNode* Child : ProjectNode->GetChildren())
		{
			if (TSharedPtr<FArticyHierarchyTreeItem> Item = BuildItem(Child, SelectableObjects, ResolveObject, PlacedIds))
			{
				RootItems.Add(Item);
			}
		}
	}

	// Keep objects pickable that the hierarchy doesn't know about, e.g. when it is outdated
	TSharedPtr<FArticyHierarchyTreeItem> MissingGroup;
	for (const TPair<FArticyId, TWeakObjectPtr<UArticyObject>>& Selectable : SelectableObjects)
	{
		if (PlacedIds.Contains(Selectable.Key))
			continue;

		if (!MissingGroup.IsValid())
		{
			MissingGroup = MakeShared<FArticyHierarchyTreeItem>();
		}

		TSharedPtr<FArticyHierarchyTreeItem> Item = MakeShared<FArticyHierarchyTreeItem>();
		Item->Id = Selectable.Key;
		Item->Object = Selectable.Value;
		Item->bSelectable = true;
		if (const UArticyObject* ArticyObject = Selectable.Value.Get())
		{
			Item->TechnicalName = ArticyObject->GetTechnicalName().ToString();
		}
		Item->Parent = MissingGroup;
		MissingGroup->Children.Add(Item);
	}

	if (MissingGroup.IsValid())
	{
		MissingGroup->Children.Sort([](const TSharedPtr<FArticyHierarchyTreeItem>& A, const TSharedPtr<FArticyHierarchyTreeItem>& B)
			{
				return A->GetLabel() < B->GetLabel();
			});
		RootItems.Add(MissingGroup);
	}

	return RootItems;
}

/**
 * @brief Builds the item for a node and its kept descendants.
 *
 * @return The item, or nullptr if neither the node nor any descendant is selectable.
 */
TSharedPtr<FArticyHierarchyTreeItem> FArticyHierarchyTreeItem::BuildItem(
	const UArticyHierarchyNode* Node,
	const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects,
	const TFunction<UArticyObject*(const FArticyId&)>& ResolveObject,
	TSet<FArticyId>& OutPlacedIds)
{
	if (!Node)
		return nullptr;

	TSharedPtr<FArticyHierarchyTreeItem> Item = MakeShared<FArticyHierarchyTreeItem>();
	Item->Id = Node->GetId();
	Item->TechnicalName = Node->GetTechnicalName();
	Item->ArticyType = Node->GetArticyType();

	for (const UArticyHierarchyNode* ChildNode : Node->GetChildren())
	{
		if (TSharedPtr<FArticyHierarchyTreeItem> Child = BuildItem(ChildNode, SelectableObjects, ResolveObject, OutPlacedIds))
		{
			Child->Parent = Item;
			Item->Children.Add(Child);
		}
	}

	if (const TWeakObjectPtr<UArticyObject>* Selectable = SelectableObjects.Find(Item->Id))
	{
		Item->bSelectable = true;
		Item->Object = *Selectable;
		OutPlacedIds.Add(Item->Id);
	}
	else if (Item->Children.Num() == 0)
	{
		return nullptr;
	}
	else if (ResolveObject)
	{
		Item->Object = ResolveObject(Item->Id);
	}

	return Item;
}

//---------------------------------------------------------------------------//

/**
 * @brief Constructs the hierarchy view.
 *
 * @param InArgs The construction arguments.
 */
void SArticyObjectHierarchyView::Construct(const FArguments& InArgs)
{
	OnObjectSelected = InArgs._OnObjectSelected;
	HighlightText = InArgs._HighlightText;

	ChildSlot
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(TreeView, STreeView<FArticyHierarchyTreeItemPtr>)
						.SelectionMode(ESelectionMode::Single)
						.TreeItemsSource(&RootItems)
						.OnGenerateRow(this, &SArticyObjectHierarchyView::OnGenerateRow)
						.OnGetChildren(this, &SArticyObjectHierarchyView::OnGetChildren)
						.OnSelectionChanged(this, &SArticyObjectHierarchyView::OnSelectionChanged)
						.OnExpansionChanged(this, &SArticyObjectHierarchyView::OnExpansionChanged)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(12.f)
				[
					SNew(STextBlock)
						.Text(this, &SArticyObjectHierarchyView::GetEmptyText)
						.Visibility(this, &SArticyObjectHierarchyView::GetEmptyTextVisibility)
						.AutoWrapText(true)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
		];
}

/**
 * @brief Rebuilds the tree from the project hierarchy of the database asset.
 *
 * @param SelectableObjects The objects that pass the picker's filters, by id.
 * @param bExpandAll Expand every item instead of restoring the user's expansion state.
 */
void SArticyObjectHierarchyView::SetSelectableObjects(const TMap<FArticyId, TWeakObjectPtr<UArticyObject>>& SelectableObjects, bool bExpandAll)
{
	const UArticyHierarchyManager* Hierarchy = nullptr;
	if (const UArticyDatabase* Database = UArticyDatabase::GetMutableOriginal().Get())
	{
		Hierarchy = Database->GetProjectHierarchy();
	}

	// The database asset only has the hierarchy once its assets were generated by an importer that stores it;
	// until then, fall back to the hierarchy kept in the import data.
	if (!Hierarchy || !Hierarchy->GetProjectNode())
	{
		const TWeakObjectPtr<UArticyImportData> ImportData = UArticyImportData::GetImportData();
		if (ImportData.IsValid())
		{
			TArray<FArticyHierarchyNodeData> Nodes;
			ImportData->GetHierarchy().BuildRuntimeNodes(Nodes);
			if (Nodes.Num() > 0)
			{
				if (!FallbackHierarchy.IsValid())
				{
					FallbackHierarchy.Reset(NewObject<UArticyHierarchyManager>(GetTransientPackage()));
				}
				FallbackHierarchy->SetSerializedNodes(MoveTemp(Nodes));
				Hierarchy = FallbackHierarchy.Get();
			}
		}
	}

	bHasHierarchy = Hierarchy && Hierarchy->GetProjectNode();

	RootItems = FArticyHierarchyTreeItem::BuildTree(Hierarchy, SelectableObjects, [](const FArticyId& Id)
		{
			return UArticyObject::FindAsset(Id);
		});

	if (!bExpansionInitialized && bHasHierarchy)
	{
		// Start out like articy's navigator, with the top-level folders open
		for (const FArticyHierarchyTreeItemPtr& Item : RootItems)
		{
			ExpandedIds.Add(Item->Id);
		}
		bExpansionInitialized = true;
	}

	ApplyExpansion(RootItems, bExpandAll);
	TreeView->RequestTreeRefresh();
}

/**
 * @brief Expands the path to an object, highlights it and scrolls it into view, without picking it.
 *
 * @param Id The id of the object to reveal.
 */
void SArticyObjectHierarchyView::RevealObject(const FArticyId& Id)
{
	TreeView->ClearHighlightedItems();
	if (Id.IsNull())
		return;

	const FArticyHierarchyTreeItemPtr Item = FindItem(RootItems, Id);
	if (!Item.IsValid())
		return;

	for (FArticyHierarchyTreeItemPtr Ancestor = Item->Parent.Pin(); Ancestor.IsValid(); Ancestor = Ancestor->Parent.Pin())
	{
		TreeView->SetItemExpansion(Ancestor, true);
	}

	// Highlight rather than select: selecting is what picks an object
	TreeView->SetItemHighlighted(Item, true);
	TreeView->RequestScrollIntoView(Item);
}

TSharedRef<ITableRow> SArticyObjectHierarchyView::OnGenerateRow(FArticyHierarchyTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FText Label = FText::FromString(Item->GetLabel());

	TSharedPtr<IToolTip> ToolTip;
	if (Item->Object.IsValid())
	{
		ToolTip = SNew(SArticyObjectToolTip).ObjectToDisplay(Item->Id);
	}
	else if (!Item->Id.IsNull())
	{
		const FText Reason = LOCTEXT("NotExportedToolTip", "Not exported");
		ToolTip = SNew(SToolTip).Text(FText::Format(LOCTEXT("NodeToolTip", "{0} ({1})\n{2}"),
			FText::FromString(Item->TechnicalName.IsEmpty() ? Item->GetLabel() : Item->TechnicalName),
			FText::FromString(Item->ArticyType),
			Reason));
	}

	return SNew(STableRow<FArticyHierarchyTreeItemPtr>, OwnerTable)
		.Padding(FMargin(0.f, 1.f))
		.ToolTip(ToolTip)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SBox)
						.WidthOverride(16.f)
						.HeightOverride(16.f)
						[
							SNew(SImage)
								.Image(GetItemIcon(*Item))
						]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(Label)
						.HighlightText(HighlightText)
						// Items that can't be picked are only the path to one, like disabled entries
						.ColorAndOpacity(Item->bSelectable ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground())
				]
		];
}

void SArticyObjectHierarchyView::OnGetChildren(FArticyHierarchyTreeItemPtr Item, TArray<FArticyHierarchyTreeItemPtr>& OutChildren) const
{
	OutChildren = Item->Children;
}

void SArticyObjectHierarchyView::OnSelectionChanged(FArticyHierarchyTreeItemPtr Item, ESelectInfo::Type SelectInfo)
{
	// Only user interaction picks an object
	if (!Item.IsValid() || SelectInfo == ESelectInfo::Direct)
		return;

	if (Item->bSelectable && Item->Object.IsValid())
	{
		OnObjectSelected.ExecuteIfBound(Item->Object.Get());
		return;
	}

	// Clicking an item that can't be picked opens or closes it instead
	if (Item->Children.Num() > 0)
	{
		TreeView->SetItemExpansion(Item, !TreeView->IsItemExpanded(Item));
	}
	TreeView->ClearSelection();
}

void SArticyObjectHierarchyView::OnExpansionChanged(FArticyHierarchyTreeItemPtr Item, bool bExpanded)
{
	if (bIsApplyingExpansion || !Item.IsValid())
		return;

	if (bExpanded)
	{
		ExpandedIds.Add(Item->Id);
	}
	else
	{
		ExpandedIds.Remove(Item->Id);
	}
}

void SArticyObjectHierarchyView::ApplyExpansion(const TArray<FArticyHierarchyTreeItemPtr>& Items, bool bExpandAll)
{
	TGuardValue<bool> Guard(bIsApplyingExpansion, true);

	for (const FArticyHierarchyTreeItemPtr& Item : Items)
	{
		if (Item->Children.Num() == 0)
			continue;

		TreeView->SetItemExpansion(Item, bExpandAll || ExpandedIds.Contains(Item->Id));
		ApplyExpansion(Item->Children, bExpandAll);
	}
}

FArticyHierarchyTreeItemPtr SArticyObjectHierarchyView::FindItem(const TArray<FArticyHierarchyTreeItemPtr>& Items, const FArticyId& Id) const
{
	for (const FArticyHierarchyTreeItemPtr& Item : Items)
	{
		if (Item->Id == Id && Item->bSelectable)
			return Item;

		if (FArticyHierarchyTreeItemPtr Found = FindItem(Item->Children, Id))
			return Found;
	}
	return nullptr;
}

FText SArticyObjectHierarchyView::GetEmptyText() const
{
	if (!bHasHierarchy && RootItems.Num() == 0)
	{
		return LOCTEXT("NoHierarchy", "No project hierarchy has been imported.\nInclude the Hierarchy in your articy export and reimport to browse it here.");
	}
	return LOCTEXT("NoMatches", "No matching objects.");
}

EVisibility SArticyObjectHierarchyView::GetEmptyTextVisibility() const
{
	return RootItems.Num() == 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

/**
 * @brief Gets the icon of an item, based on its articy type.
 *
 * @param Item The item.
 * @return The brush to show.
 */
const FSlateBrush* SArticyObjectHierarchyView::GetItemIcon(const FArticyHierarchyTreeItem& Item) const
{
	const ISlateStyle& Style = FArticyEditorStyle::Get();

	if (!Item.ArticyType.IsEmpty())
	{
		if (const FSlateBrush* TypeBrush = Style.GetOptionalBrush(FName(*FString::Printf(TEXT("ArticyImporter.Type.%s.16"), *Item.ArticyType)), nullptr, nullptr))
		{
			return TypeBrush;
		}
	}

	if (const UArticyObject* ArticyObject = Item.Object.Get())
	{
		return UserInterfaceHelperFunctions::GetArticyTypeImage(ArticyObject, UserInterfaceHelperFunctions::Small);
	}

	if (Item.ArticyType.StartsWith(TEXT("ProjectSettings")))
	{
		return Style.GetBrush("ArticyImporter.Type.ProjectSettings.16");
	}

	if (Item.ArticyType.Contains(TEXT("Template")))
	{
		return Style.GetBrush("ArticyImporter.Type.Template.16");
	}

	if (Item.Id.IsNull() || Item.ArticyType.Contains(TEXT("Folder")) || Item.Children.Num() > 0)
	{
		return Style.GetBrush("ArticyImporter.Type.SystemFolder.16");
	}

	return Style.GetBrush("ArticyImporter.ArticyDraft.16");
}

#undef LOCTEXT_NAMESPACE
