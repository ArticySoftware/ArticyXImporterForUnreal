//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyImportStatus.h"
#include "ArticyPluginUpdateChecker.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class SSearchBox;
class UArticyObject;

/**
 * @brief An object in the list of the database view.
 */
struct ARTICYEDITOR_API FArticyDatabaseViewItem
{
	TWeakObjectPtr<UArticyObject> Object;
	FString DisplayName;
	FString TechnicalName;
	FString Type;
	/** The id as hex. */
	FString Id;

	/**
	 * @param InObject The object to list.
	 * @param ClassPrefix The project's technical name, dropped from the class name of an object without an articy type.
	 */
	explicit FArticyDatabaseViewItem(UArticyObject* InObject, const FString& ClassPrefix = FString());

	/** @brief Returns true for an id, or a part of a name, the type or the text of the object. */
	bool Matches(const FString& Filter) const;

	/** @brief Gets the value shown in a column. */
	const FString& GetColumnText(const FName& Column) const;
};

using FArticyDatabaseViewItemPtr = TSharedPtr<FArticyDatabaseViewItem>;

/**
 * @brief Shows the imported articy database: the import state, all objects and the properties of the selected one.
 */
class ARTICYEDITOR_API SArticyDatabaseView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArticyDatabaseView) : _bLiveData(true) {}
		/** Whether the view fills itself from the project. Tests turn this off and set the data. */
		SLATE_ARGUMENT(bool, bLiveData)
	SLATE_END_ARGS()

	virtual ~SArticyDatabaseView();

	void Construct(const FArguments& InArgs);

	/** @brief Reads the import state and the objects from the project again. */
	void Refresh();

	void SetImportStatus(const FArticyImportStatus& InStatus);
	void SetUpdateStatus(const FArticyPluginUpdateStatus& InStatus);
	void SetObjects(const TArray<UArticyObject*>& Objects);
	void SetFilterText(const FText& InFilterText);
	void SelectObject(const UArticyObject* Object);

	const FArticyImportStatus& GetImportStatus() const { return ImportStatus; }
	int32 GetNumObjects() const { return AllItems.Num(); }
	int32 GetNumVisibleObjects() const { return VisibleItems.Num(); }
	UArticyObject* GetSelectedObject() const;
	bool IsHelpBoxVisible() const { return !ImportStatus.IsUpToDate(); }

	/**
	 * @brief Collects the items matching a filter, in the order of a column.
	 * @param Items The items to pick from.
	 * @param Filter The filter text; empty for all items.
	 * @param SortColumn The column to sort by.
	 * @param bAscending The sort direction.
	 * @return The matching items.
	 */
	static TArray<FArticyDatabaseViewItemPtr> FilterAndSort(const TArray<FArticyDatabaseViewItemPtr>& Items, const FString& Filter, const FName& SortColumn, bool bAscending);

	static const FName NameColumn;
	static const FName TechnicalNameColumn;
	static const FName TypeColumn;
	static const FName IdColumn;

private:
	TSharedRef<SWidget> CreateHeader();
	TSharedRef<SWidget> CreateHelpBox();
	TSharedRef<SWidget> CreateObjectList();
	TSharedRef<SWidget> CreateInspector();
	TSharedRef<SWidget> CreateLocalizationBar();

	TSharedRef<ITableRow> OnGenerateRow(FArticyDatabaseViewItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnSelectionChanged(FArticyDatabaseViewItemPtr Item, ESelectInfo::Type SelectInfo);
	void OnSortModeChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	EColumnSortMode::Type GetColumnSortMode(FName Column) const;

	/** Applies the filter and the sorting to the list. */
	void UpdateVisibleItems();
	void RunImportAction(EArticyImportAction Action);
	FReply OnUpdateLabelClicked() const;

	const FArticyPluginUpdateStatus& GetUpdateStatus() const;
	FText GetLanguageText() const;
	bool IsCurrentLanguageExported() const;

private:
	bool bLiveData = true;

	FArticyImportStatus ImportStatus;
	/** Set by SetUpdateStatus; otherwise the update checker of the module is shown. */
	TOptional<FArticyPluginUpdateStatus> UpdateStatusOverride;

	TArray<FArticyDatabaseViewItemPtr> AllItems;
	TArray<FArticyDatabaseViewItemPtr> VisibleItems;
	FText FilterText;
	FName SortColumn;
	bool bSortAscending = true;

	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<FArticyDatabaseViewItemPtr>> ListView;
	TSharedPtr<IDetailsView> DetailsView;

	FDelegateHandle AssetsGeneratedHandle;
	FDelegateHandle ImportFinishedHandle;
};
