//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "ArticyObject.h"
#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/STableViewBase.h>
#include <Widgets/Views/STableRow.h>
#include <IContentBrowserSingleton.h>
#include <SAssetSearchBox.h>
#include <Types/SlateEnums.h>
#include <Misc/TextFilterExpressionEvaluator.h>
#include "Widgets/Input/SComboButton.h"
#include "Slate/ArticyFilterHelpers.h"
#include "ClassViewerModule.h"

#define LOCTEXT_NAMESPACE "ArticyObjectAssetPicker"

namespace FArticyObjectAssetPicketConstants {

	/**
	 * @brief Constants for Articy object asset picker.
	 */
	const FVector2D TileSize(96.f, 96.f);
	const int32 ThumbnailPadding = 2;

}

/**
 * @brief A widget for picking Articy object assets.
 *
 * This widget allows users to search and filter Articy objects, displaying them in a tile view for easy selection.
 */
class ARTICYEDITOR_API SArticyObjectAssetPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArticyObjectAssetPicker) :
		_CurrentClassRestriction(UArticyObject::StaticClass()),
		_TopLevelClassRestriction(UArticyObject::StaticClass()),
		_bExactClass(false),
		_bExactClassEditable(true),
		_bClassFilterEditable(true)
		{}
		SLATE_EVENT(FOnAssetSelected, OnArticyObjectSelected)
		SLATE_ARGUMENT(UClass*, CurrentClassRestriction)
		SLATE_ATTRIBUTE(UClass*, TopLevelClassRestriction)
		SLATE_ATTRIBUTE(bool, bExactClass)
		SLATE_ATTRIBUTE(bool, bExactClassEditable)
		SLATE_ATTRIBUTE(bool, bClassFilterEditable)
		SLATE_EVENT(FOnClassPicked, OnClassPicked)
	SLATE_END_ARGS()

	~SArticyObjectAssetPicker();

	/**
	 * @brief Constructs the Articy Object Asset Picker widget.
	 *
	 * @param InArgs The construction arguments for the widget.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Ticks the widget for updates.
	 *
	 * @param AllottedGeometry The geometry of the widget.
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time elapsed since the last tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/**
	 * @brief Requests a slow full list refresh.
	 */
	void RequestSlowFullListRefresh();

	/**
	 * @brief Selects an asset in the asset picker.
	 *
	 * @param AssetItem The selected asset item.
	 * @param SelectInfo The selection information.
	 */
	void SelectAsset(TWeakObjectPtr<UArticyObject> AssetItem, ESelectInfo::Type SelectInfo) const;

private:
	/**
	 * @brief Creates internal UI widgets for the asset picker.
	 */
	void CreateInternalWidgets();

	/**
	 * @brief Copies the property value of the specified ArticyId to the clipboard.
	 *
	 * @param Id The ArticyId to copy.
	 */
	void OnCopyProperty(FArticyId Id) const;

	/**
	 * @brief Creates a class picker widget for selecting the class type.
	 *
	 * @return A shared reference to the created class picker widget.
	 */
	TSharedRef<SWidget> CreateClassPicker();

	/**
	 * @brief Called when the "Exact Class" checkbox state changes.
	 *
	 * @param NewState The new checkbox state.
	 */
	void OnExactClassCheckBoxChanged(ECheckBoxState NewState);

	/**
	 * @brief Handles the class picked event when a class is selected.
	 *
	 * @param InChosenClass The chosen class.
	 */
	void OnClassPicked_Func(UClass* InChosenClass);

	/**
	 * @brief Gets the name of the currently chosen class.
	 *
	 * @return The name of the currently chosen class.
	 */
	FText GetChosenClassName() const;

	/**
	 * @brief Creates a tile view widget for displaying Articy objects.
	 *
	 * @param Entity The Articy object entity to display.
	 * @param OwnerTable The owner table for the widget.
	 * @return A shared reference to the created tile view widget.
	 */
	TSharedRef<class ITableRow> MakeTileViewWidget(TWeakObjectPtr<UArticyObject> Entity, const TSharedRef<STableViewBase>& OwnerTable) const;

	/**
	 * @brief Gets the height of the tile view.
	 *
	 * @return The height of the tile view.
	 */
	float GetTileViewHeight() const;

	/**
	 * @brief Gets the width of the tile view.
	 *
	 * @return The width of the tile view.
	 */
	float GetTileViewWidth() const;

	/**
	 * @brief Clears the selected asset.
	 */
	void OnClear() const;

	/**
	 * @brief Called when the search box text changes.
	 *
	 * @param InSearchText The new search text.
	 */
	void OnSearchBoxChanged(const FText& InSearchText) const;

	/**
	 * @brief Called when the search box text is committed.
	 *
	 * @param InSearchText The committed search text.
	 * @param CommitInfo The commit information.
	 */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo) const;

	/**
	 * @brief Refreshes the source items for the asset picker.
	 */
	void RefreshSourceItems();

	/**
	 * @brief Sets the search box text for filtering assets.
	 *
	 * @param InSearchText The search text to set.
	 */
	void SetSearchBoxText(const FText& InSearchText) const;

	/**
	 * @brief Called when frontend filters change.
	 */
	void OnFrontendFiltersChanged();

	/**
	 * @brief Tests an asset item against frontend filters.
	 *
	 * @param Item The asset item to test.
	 * @return True if the item passes all filters, false otherwise.
	 */
	bool TestAgainstFrontendFilters(const FAssetData& Item) const;

	/**
	 * @brief Focuses the search field widget.
	 *
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time elapsed since the last tick.
	 * @return An EActiveTimerReturnType indicating whether to continue or stop the active timer.
	 */
	EActiveTimerReturnType FocusSearchField(double InCurrentTime, float InDeltaTime) const;

private: // Slate Attributes
	FOnAssetSelected OnAssetSelected; //!< Delegate for handling asset selection events.
	FOnClassPicked OnClassPicked; //!< Delegate for handling class picking events.
	TAttribute<UClass*> TopLevelClassRestriction; //!< Attribute representing the top-level class restriction for the asset picker.
	UClass* CurrentClassRestriction = nullptr; //!< Pointer to the current class restriction.
	TAttribute<bool> bExactClass; //!< Attribute indicating whether the picker is restricted to exact classes.
	TAttribute<bool> bExactClassEditable; //!< Attribute indicating whether the exact class option is editable.
	TAttribute<bool> bClassFilterEditable; //!< Attribute indicating whether the class filter is editable.

private: // Widgets
	TSharedPtr<SAssetSearchBox> SearchField; //!< Shared pointer to the search box widget.
	TSharedPtr<SWidget> AssetViewContainer; //!< Shared pointer to the asset view container widget.
	TSharedPtr<SListView<TWeakObjectPtr<UArticyObject>>> AssetView; //!< Shared pointer to the asset view list widget.
	TSharedPtr<SHorizontalBox> FilterBox; //!< Shared pointer to the filter box widget.
	TSharedPtr<SComboButton> ClassFilterButton; //!< Shared pointer to the class filter button widget.

private: // Internal Data
	TSharedPtr<FArticyObjectFilterCollectionType> FrontendFilters; //!< Shared pointer to the frontend filter collection for Articy objects.
	TSharedPtr<FArticyClassRestrictionFilter> ClassFilter; //!< Shared pointer to the class restriction filter.
	TSharedPtr<FFrontendFilter_ArticyObject> ArticyObjectFilter; //!< Shared pointer to the frontend filter for Articy objects.

	TArray<FAssetData> ArticyPackageDataAssets; //!< Array of asset data for Articy packages.
	TArray<TWeakObjectPtr<UArticyObject>> FilteredObjects; //!< Array of filtered Articy objects.
	bool bSlowFullListRefreshRequested = false; //!< Flag indicating whether a slow full list refresh is requested.
};

#undef LOCTEXT_NAMESPACE
