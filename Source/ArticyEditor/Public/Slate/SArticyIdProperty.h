//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Slate/AssetPicker/SArticyObjectTileView.h"
#include "PropertyHandle.h"
#include "Customizations/ArticyEditorCustomizationManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "ContentBrowserDelegates.h"
#include "AssetPicker/SArticyObjectAssetPicker.h"

namespace ArticyRefPropertyConstants
{
	const FVector2D ThumbnailSize(64, 64);
	const FMargin ThumbnailPadding(2, 2);
}

/**
 * This combo button is 1:1 the same as the normal SComboButton except that its Tick function does not try to Resize the menu.
 * Instead, it only moves the Window. This fixes a problem where the menu would continuously be resized with 1 pixel difference which causes flickering
 * in some edge cases (zoom level -1 and a menu that wasn't fixed size 300x300 like the default asset picker (300x350 would also flicker)
 */
class SFixedSizeMenuComboButton : public SComboButton
{
protected:
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
};

/**
 * A widget for displaying and selecting Articy object IDs.
 *
 * REFERENCE: SPropertyEditorAsset, which is the normal asset selection widget.
 */
class ARTICYEDITOR_API SArticyIdProperty : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SArticyIdProperty)
		: _ArticyIdToDisplay(FArticyId())
		, _TopLevelClassRestriction(UArticyObject::StaticClass())
		, _HighExtender(nullptr)
		, _CopyAction()
		, _PasteAction()
		, _bExactClass(false)
		, _bExactClassEditable(true)
		, _bIsReadOnly(false)
		{}
		SLATE_ATTRIBUTE(FArticyId, ArticyIdToDisplay)
		SLATE_EVENT(FOnArticyIdChanged, OnArticyIdChanged)
		SLATE_ATTRIBUTE(UClass*, TopLevelClassRestriction)
		SLATE_ARGUMENT(TSharedPtr<FExtender>, HighExtender)
		SLATE_ARGUMENT(FUIAction, CopyAction)
		SLATE_ARGUMENT(FUIAction, PasteAction)
		SLATE_ATTRIBUTE(bool, bExactClass)
		SLATE_ATTRIBUTE(bool, bExactClassEditable)
		SLATE_ATTRIBUTE(bool, bIsReadOnly)
	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * Tick function for the widget, updates the widget state.
	 *
	 * @param AllottedGeometry The allotted geometry for this widget.
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time elapsed since the last tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/**
	 * Creates the internal widgets for the ArticyId property editor.
	 */
	void CreateInternalWidgets();

private:
	TAttribute<FArticyId> ArticyIdToDisplay;
	FOnArticyIdChanged OnArticyIdChanged;
	TAttribute<UClass*> TopLevelClassRestriction;
	TAttribute<bool> bExactClass;
	TAttribute<bool> bExactClassEditable;
	TAttribute<bool> bIsReadOnly;

	// the articy object this widget currently represents
	TWeakObjectPtr<UArticyObject> CachedArticyObject = nullptr;
	mutable FArticyId CachedArticyId = FArticyId();

	TSharedPtr<SHorizontalBox> ChildBox;
	TSharedPtr<SArticyObjectTileView> TileView;
	TSharedPtr<SBox> TileContainer;
	TSharedPtr<SBorder> ThumbnailBorder;
	TSharedPtr<FSlateBrush> ImageBrush;
	TSharedPtr<SWidget> ComboButton;
	/** The high box is used to register additional widgets when creating an SArticyIdProperty */
	TSharedPtr<SHorizontalBox> CustomizationButtonBox_High;
	/** The low box is used for the articyref customization system. */
	TSharedPtr<SHorizontalBox> CustomizationButtonBox_Low;

private:
	/**
	 * Updates the internal values and fires delegates.
	 *
	 * @param NewId The new Articy ID to update.
	 */
	void Update(const FArticyId& NewId);

	/**
	 * Updates the widget, including customizations.
	 */
	void UpdateWidget();

	/**
	 * Applies a single customization to the ArticyRef.
	 *
	 * @param Customization The customization info to apply.
	 */
	void ApplyArticyRefCustomization(const FArticyIdPropertyWidgetCustomizationInfo& Customization);

	/**
	 * Applies multiple customizations to the ArticyRef.
	 *
	 * @param Customizations The array of customization info to apply.
	 */
	void ApplyArticyRefCustomizations(const TArray<FArticyIdPropertyWidgetCustomizationInfo>& Customizations);

private:
	/**
	 * Creates an Articy object asset picker widget.
	 *
	 * @return A shared reference to the created widget.
	 */
	TSharedRef<SWidget> CreateArticyObjectAssetPicker();

	/**
	 * Determines if the class filter is editable.
	 *
	 * @return true if the class filter is editable, false otherwise.
	 */
	bool IsClassFilterEditable() const;

	/**
	 * Called when an Articy object is picked from the asset picker.
	 *
	 * @param ArticyObjectData The asset data of the picked object.
	 */
	void OnArticyObjectPicked(const FAssetData& ArticyObjectData) const;

	/**
	 * Handles the click event for the Articy button.
	 *
	 * @return The reply result after handling the click event.
	 */
	FReply OnArticyButtonClicked() const;

	/**
	 * Handles the double-click event on the asset thumbnail.
	 *
	 * @param InMyGeometry The geometry of the clicked widget.
	 * @param InMouseEvent The mouse event data.
	 * @return The reply result after handling the double-click event.
	 */
	FReply OnAssetThumbnailDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) const;

	/**
	 * Gets the display name of the currently selected Articy object.
	 *
	 * @return The display name as a text object.
	 */
	FText OnGetArticyObjectDisplayName() const;

	/**
	 * Gets the current object ID of the selected Articy object.
	 *
	 * @return The Articy ID of the current object.
	 */
	FArticyId GetCurrentObjectID() const;

	/**
	 * Determines if the property can be activated.
	 *
	 * @return true if the property can be activated, false otherwise.
	 */
	bool CanActivate() const { return !bIsReadOnly.Get(); }

	/**
	 * Copies the current property value to the clipboard.
	 */
	void OnCopyProperty() const;

	/**
	 * Pastes the property value from the clipboard.
	 */
	void OnPasteProperty();

	/**
	 * Determines if the property value can be pasted.
	 *
	 * @return true if the property value can be pasted, false otherwise.
	 */
	bool CanPasteProperty() const;

private:
	FUIAction CopyAction;
	FUIAction PasteAction;
	/** The current customizations are cached in here to achieve ownership */
	TArray<TSharedPtr<IArticyIdPropertyWidgetCustomization>> ActiveCustomizations;

	TSharedPtr<FExtender> CustomizationHighExtender;
	/** The customization extenders of the currently active customizations */
	TArray<TSharedPtr<FExtender>> ArticyIdCustomizationExtenders;

};
