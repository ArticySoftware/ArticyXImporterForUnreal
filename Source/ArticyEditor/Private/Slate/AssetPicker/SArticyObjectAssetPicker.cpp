//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/AssetPicker/SArticyObjectAssetPicker.h"
#include "ArticyObject.h"
#include "EditorStyleSet.h"
#include "GenericPlatform/ICursor.h"
#include "Slate/AssetPicker/SArticyObjectTileView.h"
#include "ArticyGlobalVariables.h"
#include <ContentBrowserModule.h>
#include "ArticyPluginSettings.h"
#include "Customizations/Details/ArticyIdCustomization.h"
#include "Types/WidgetActiveTimerDelegate.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "ArticyEditorModule.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "ArticyObjectAssetPicker"

/**
 * @brief Destructor for SArticyObjectAssetPicker.
 */
SArticyObjectAssetPicker::~SArticyObjectAssetPicker()
{
}

/**
 * @brief Constructs the Articy Object Asset Picker widget.
 *
 * This method sets up the UI components and filtering logic for the asset picker widget, including search and filter functionality.
 *
 * @param InArgs The construction arguments for the widget.
 */
void SArticyObjectAssetPicker::Construct(const FArguments& InArgs)
{
	OnAssetSelected = InArgs._OnArticyObjectSelected;
	OnClassPicked = InArgs._OnClassPicked;
	CurrentClassRestriction = InArgs._CurrentClassRestriction;
	TopLevelClassRestriction = InArgs._TopLevelClassRestriction;
	bExactClass = InArgs._bExactClass;
	bExactClassEditable = InArgs._bExactClassEditable;
	bClassFilterEditable = InArgs._bClassFilterEditable;

	if (!CurrentClassRestriction->IsChildOf(TopLevelClassRestriction.Get()))
	{
		CurrentClassRestriction = TopLevelClassRestriction.Get();
	}

	SetCursor(EMouseCursor::Hand);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	const bool bCloseSelfOnly = true;

	FrontendFilters = MakeShareable(new FArticyObjectFilterCollectionType());
	FrontendFilters->OnChanged().AddSP(this, &SArticyObjectAssetPicker::OnFrontendFiltersChanged);
	ArticyObjectFilter = MakeShareable(new FFrontendFilter_ArticyObject());
	ClassFilter = MakeShareable(new FArticyClassRestrictionFilter(CurrentClassRestriction, bExactClass.Get()));

	CreateInternalWidgets();

	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly);
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentAssetOperationsHeader", "Current Asset"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearAsset", "Clear"),
			LOCTEXT("ClearAsset_ToolTip", "Clears the articy object reference"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SArticyObjectAssetPicker::OnClear))
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		MenuBuilder.AddWidget(SearchField.ToSharedRef(), FText::GetEmpty(), true);
		MenuBuilder.AddWidget(AssetViewContainer.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("FilterHeader", "Filter"));
	{
		MenuBuilder.AddWidget(FilterBox.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	this->ChildSlot
		[
			MenuBuilder.MakeWidget()
		];

	// focus the search field next frame
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SArticyObjectAssetPicker::FocusSearchField));

	// filter has be added after the widgets have been created since adding will trigger a callback function that accesses the widgets (FrontEndFilters OnChanged, which causes a refresh)
	FrontendFilters->Add(ClassFilter);

	// refresh is unnecessary due to the new filter addition causing a refresh anyways, but keep it for clarity's sake
	RequestSlowFullListRefresh();
}

/**
 * @brief Ticks the widget for updates.
 *
 * This method is called every frame to update the widget state, such as refreshing the asset list if needed.
 *
 * @param AllottedGeometry The geometry of the widget.
 * @param InCurrentTime The current time.
 * @param InDeltaTime The time elapsed since the last tick.
 */
void SArticyObjectAssetPicker::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// reference: assetview.cpp:1189
	if (bSlowFullListRefreshRequested)
	{
		if (AssetView.IsValid())
		{
			AssetView->ClearSelection();
		}

		AsyncTask(ENamedThreads::GameThread, [this]()
			{
				RefreshSourceItems();
			});
		bSlowFullListRefreshRequested = false;
	}
}

/**
 * @brief Creates internal UI widgets for the asset picker.
 *
 * This method initializes the search box, asset view, and class filter button.
 */
void SArticyObjectAssetPicker::CreateInternalWidgets()
{
	SAssignNew(SearchField, SAssetSearchBox)
		.HintText(NSLOCTEXT("ContentBrowser", "SearchBoxHint", "Search Assets"))
		.OnTextChanged(this, &SArticyObjectAssetPicker::OnSearchBoxChanged)
		.OnTextCommitted(this, &SArticyObjectAssetPicker::OnSearchBoxCommitted)
		.DelayChangeNotificationsWhileTyping(true);

	SAssignNew(AssetViewContainer, SBox)
		.WidthOverride(325)
		.HeightOverride(325)
		[
			SAssignNew(AssetView, STileView<TWeakObjectPtr<UArticyObject>>)
				.SelectionMode(ESelectionMode::Single)
				.ListItemsSource(&FilteredObjects)
				.OnGenerateTile(this, &SArticyObjectAssetPicker::MakeTileViewWidget)
				.OnSelectionChanged(this, &SArticyObjectAssetPicker::SelectAsset)
				.OnItemToString_Debug_Lambda([](TWeakObjectPtr<UArticyObject> Item)
					{
						if (!Item.IsValid())
						{
							return FString(TEXT("Invalid WeakPtr"));
						}

						UArticyObject* Obj = Item.Get();

						if (!IsValid(Obj))
						{
							return FString(TEXT("Destroyed UObject"));
						}

						return Obj->GetName();
					})
				.ItemHeight(this, &SArticyObjectAssetPicker::GetTileViewHeight)
				.ItemWidth(this, &SArticyObjectAssetPicker::GetTileViewWidth)
				.ItemAlignment(EListItemAlignment::EvenlyDistributed)
		];

	ClassFilterButton = SNew(SComboButton)
		.OnGetMenuContent(this, &SArticyObjectAssetPicker::CreateClassPicker)
		.IsEnabled_Lambda([this]() -> bool
			{
				return bClassFilterEditable.Get();
			})
		.ButtonContent()
				[
					SNew(STextBlock)
						.Text(this, &SArticyObjectAssetPicker::GetChosenClassName)
				];

			SAssignNew(FilterBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.AutoWidth()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Padding(3.f)
						[
							SNew(STextBlock).Text(FText::FromString("Exact Class "))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SCheckBox)
								.IsEnabled(bExactClassEditable)
#if __cplusplus >= 202002L
								.IsChecked_Lambda([=, this]()
#else
									.IsChecked_Lambda([=]()
#endif
										{
											return bExactClass.Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
										})
									.OnCheckStateChanged(this, &SArticyObjectAssetPicker::OnExactClassCheckBoxChanged)
						]
				]
									+ SHorizontalBox::Slot()
									.FillWidth(1.f)
									[
										SNew(SSpacer)
									]
									+ SHorizontalBox::Slot()
									.VAlign(VAlign_Center)
									.HAlign(HAlign_Right)
									.AutoWidth()
									.MaxWidth(200.f)
									[
										ClassFilterButton.ToSharedRef()
									];
}

/**
 * @brief Copies the property value of the specified ArticyId to the clipboard.
 *
 * @param Id The ArticyId to copy.
 */
void SArticyObjectAssetPicker::OnCopyProperty(FArticyId Id) const
{
	FString ValueString = Id.ToString();
	FPlatformApplicationMisc::ClipboardCopy(*ValueString);
}

/**
 * @brief Creates a class picker widget for selecting the class type.
 *
 * @return A shared reference to the created class picker widget.
 */
TSharedRef<SWidget> SArticyObjectAssetPicker::CreateClassPicker()
{
	FClassViewerInitializationOptions ClassViewerConfig;
	ClassViewerConfig.DisplayMode = EClassViewerDisplayMode::ListView;
	ClassViewerConfig.bAllowViewOptions = true;
#if ENGINE_MAJOR_VERSION >= 5
	ClassViewerConfig.ClassFilters.Add(MakeShareable(new FArticyRefClassFilter(TopLevelClassRestriction.Get(), false)));
#else
	ClassViewerConfig.ClassFilter = MakeShareable(new FArticyRefClassFilter(TopLevelClassRestriction.Get(), false));
#endif

	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(
		ClassViewerConfig, FOnClassPicked::CreateRaw(this, &SArticyObjectAssetPicker::OnClassPicked_Func));
}

/**
 * @brief Handles the class picked event when a class is selected.
 *
 * @param InChosenClass The chosen class.
 */
void SArticyObjectAssetPicker::OnClassPicked_Func(UClass* InChosenClass)
{
	CurrentClassRestriction = InChosenClass;
	ClassFilter->UpdateFilteredClass(CurrentClassRestriction);
	ClassFilterButton->SetIsOpen(false, false);

	OnClassPicked.ExecuteIfBound(InChosenClass);
}

/**
 * @brief Gets the name of the currently chosen class.
 *
 * @return The name of the currently chosen class.
 */
FText SArticyObjectAssetPicker::GetChosenClassName() const
{
	if (CurrentClassRestriction)
	{
		return FText::FromString(CurrentClassRestriction->GetName());
	}

	return FText::FromString("None");
}

/**
 * @brief Creates a tile view widget for displaying Articy objects.
 *
 * @param Entity The Articy object entity to display.
 * @param OwnerTable The owner table for the widget.
 * @return A shared reference to the created tile view widget.
 */
TSharedRef<ITableRow> SArticyObjectAssetPicker::MakeTileViewWidget(
	TWeakObjectPtr<UArticyObject> Entity,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	// Always create the row first
	TSharedRef<STableRow<TWeakObjectPtr<UArticyObject>>> TableRowWidget =
		SNew(STableRow<TWeakObjectPtr<UArticyObject>>, OwnerTable)
#if ENGINE_MAJOR_VERSION == 4
		.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
#endif
		.Cursor(EMouseCursor::GrabHand)
		.Padding(3.f);

	// Validate before doing anything else
	if (!Entity.IsValid())
	{
		TableRowWidget->SetContent(
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Invalid Item")))
		);

		return TableRowWidget;
	}

	UArticyObject* SafeObject = Entity.Get();

	if (!IsValid(SafeObject))
	{
		TableRowWidget->SetContent(
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Destroyed Object")))
		);

		return TableRowWidget;
	}

	// safe to use from here
	const FArticyId SafeId = SafeObject->GetId();

	FUIAction CopyAction;
	CopyAction.ExecuteAction = FExecuteAction::CreateSP(
		this,
		&SArticyObjectAssetPicker::OnCopyProperty,
		SafeId
	);

	// create the new tile view; the object to display is fixed so it can't change without the asset picker being recreated.
	TSharedRef<SArticyObjectTileView> Item =
		SNew(SArticyObjectTileView)
		.bIsReadOnly(true)
		.CopyAction(CopyAction)
		.ArticyIdToDisplay(SafeId)
		.ThumbnailSize(FArticyObjectAssetPicketConstants::TileSize)
		.ThumbnailPadding(FArticyObjectAssetPicketConstants::ThumbnailPadding);

	TableRowWidget->SetContent(Item);

	return TableRowWidget;
}

/**
 * @brief Gets the height of the tile view.
 *
 * @return The height of the tile view.
 */
float SArticyObjectAssetPicker::GetTileViewHeight() const
{
	return FArticyObjectAssetPicketConstants::TileSize.Y + 2 * FArticyObjectAssetPicketConstants::ThumbnailPadding;
}

/**
 * @brief Gets the width of the tile view.
 *
 * @return The width of the tile view.
 */
float SArticyObjectAssetPicker::GetTileViewWidth() const
{
	return FArticyObjectAssetPicketConstants::TileSize.X + 2 * FArticyObjectAssetPicketConstants::ThumbnailPadding;
}

/**
 * @brief Clears the selected asset.
 *
 * This method clears the current asset selection, effectively deselecting any selected asset.
 */
void SArticyObjectAssetPicker::OnClear() const
{
	SelectAsset(nullptr, ESelectInfo::Direct);
}

/**
 * @brief Refreshes the source items for the asset picker.
 *
 * This method retrieves and filters Articy package assets to display in the asset picker.
 */
void SArticyObjectAssetPicker::RefreshSourceItems()
{
	ArticyPackageDataAssets.Reset();
	FilteredObjects.Reset();
	TArray<TWeakObjectPtr<UArticyObject>> NewFilteredObjects;

	// Load the asset registry module
	static const FName AssetRegistryName(TEXT("AssetRegistry"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		AssetRegistryName);

	// retrieve all articy packages
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
	AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetClassPathName(), ArticyPackageDataAssets);
#else
	AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetFName(), ArticyPackageDataAssets);
#endif

	// test the contained articy objects against the frontend filters
	for (const FAssetData& ArticyPackageAssetData : ArticyPackageDataAssets)
	{
		UArticyPackage* ArticyPackage = Cast<UArticyPackage>(ArticyPackageAssetData.GetAsset());
		if (!ArticyPackage)
		{
			continue;
		}

		for (const TWeakObjectPtr<UArticyObject> ArticyObject : ArticyPackage->GetAssets())
		{
			if (!ArticyObject.IsValid())
			{
				continue;
			}

			const FArticyId Id = ArticyObject->GetId();
			UArticyObject* ResolvedObject = UArticyObject::FindAsset(Id);

			// Skip unresolved references
			if (!ResolvedObject || !IsValid(ResolvedObject))
			{
				continue;
			}

			FAssetData AssetItem(ResolvedObject);

			if (TestAgainstFrontendFilters(AssetItem))
			{
				NewFilteredObjects.Add(ResolvedObject);
			}
		}
	}

	// swap only once
	FilteredObjects = MoveTemp(NewFilteredObjects);

	// extra safety cleanup
	FilteredObjects.RemoveAll([](const TWeakObjectPtr<UArticyObject>& Obj)
		{
			return !Obj.IsValid();
		});

	if (AssetView.IsValid())
	{
		AssetView->RequestListRefresh();
	}
}

/**
 * @brief Sets the search box text for filtering assets.
 *
 * @param InSearchText The search text to set.
 */
void SArticyObjectAssetPicker::SetSearchBoxText(const FText& InSearchText) const
{
	// Update the filter text only if it has actually changed, including case sensitivity (as operators are case sensitive)
	if (!InSearchText.ToString().Equals(ArticyObjectFilter->GetRawFilterText().ToString(), ESearchCase::CaseSensitive))
	{
		ArticyObjectFilter->SetRawFilterText(InSearchText);

		// add or remove the filter depending on whether the search field has content or not
		if (InSearchText.IsEmpty())
		{
			FrontendFilters->Remove(ArticyObjectFilter);
		}
		else
		{
			FrontendFilters->Add(ArticyObjectFilter);
		}
	}
}

/**
 * @brief Called when frontend filters change.
 *
 * This method refreshes the source items to reflect the updated filters.
 */
void SArticyObjectAssetPicker::OnFrontendFiltersChanged()
{
	RequestSlowFullListRefresh();
}

/**
 * @brief Tests an asset item against frontend filters.
 *
 * @param Item The asset item to test.
 * @return True if the item passes all filters, false otherwise.
 */
bool SArticyObjectAssetPicker::TestAgainstFrontendFilters(const FAssetData& Item) const
{
	if (FrontendFilters.IsValid() && !FrontendFilters->PassesAllFilters(Item))
	{
		return false;
	}

	return true;
}

/**
 * @brief Focuses the search field widget.
 *
 * This method sets the keyboard focus to the search field widget.
 *
 * @param InCurrentTime The current time.
 * @param InDeltaTime The time elapsed since the last tick.
 * @return An EActiveTimerReturnType indicating whether to continue or stop the active timer.
 */
EActiveTimerReturnType SArticyObjectAssetPicker::FocusSearchField(double InCurrentTime, float InDeltaTime) const
{
	if (!SearchField.IsValid())
	{
		return EActiveTimerReturnType::Continue;
	}

	FWidgetPath WidgetToFocusPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchField.ToSharedRef(), WidgetToFocusPath);
	FSlateApplication::Get().SetKeyboardFocus(WidgetToFocusPath, EFocusCause::SetDirectly);
	WidgetToFocusPath.GetWindow()->SetWidgetToFocusOnActivate(SearchField);

	return EActiveTimerReturnType::Stop;
}

/**
 * @brief Requests a slow full list refresh.
 *
 * This method sets a flag to request a full refresh of the asset list.
 */
void SArticyObjectAssetPicker::RequestSlowFullListRefresh()
{
	bSlowFullListRefreshRequested = true;
}

/**
 * @brief Selects an asset in the asset picker.
 *
 * @param AssetItem The selected asset item.
 * @param SelectInfo The selection information.
 */
void SArticyObjectAssetPicker::SelectAsset(TWeakObjectPtr<UArticyObject> AssetItem, ESelectInfo::Type SelectInfo) const
{
	if (!AssetItem.IsValid())
	{
		return; // prevent bad selection
	}

	const FAssetData NewAsset(AssetItem.Get());
	OnAssetSelected.ExecuteIfBound(NewAsset);
	FSlateApplication::Get().DismissAllMenus();
}

/**
 * @brief Called when the "Exact Class" checkbox state changes.
 *
 * @param NewState The new checkbox state.
 */
void SArticyObjectAssetPicker::OnExactClassCheckBoxChanged(ECheckBoxState NewState)
{
	bExactClass = NewState == ECheckBoxState::Checked;
	ClassFilter->UpdateExactClass(bExactClass.Get());
}

/**
 * @brief Called when the search box text changes.
 *
 * @param InSearchText The new search text.
 */
void SArticyObjectAssetPicker::OnSearchBoxChanged(const FText& InSearchText) const
{
	SetSearchBoxText(InSearchText);
}

/**
 * @brief Called when the search box text is committed.
 *
 * @param InSearchText The committed search text.
 * @param CommitInfo The commit information.
 */
void SArticyObjectAssetPicker::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo) const
{
	SetSearchBoxText(InSearchText);
}

#undef LOCTEXT_NAMESPACE
