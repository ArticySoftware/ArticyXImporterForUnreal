//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Slate/AssetPicker/SArticyObjectTileView.h"
#include "Customizations/ArticyEditorCustomizationManager.h"
#include "SArticyIdProperty.h"
#include "AssetPicker/SArticyObjectAssetPicker.h"

/**
 *  REFERENCE: SPropertyEditorAsset, which is the normal asset selection widget
 */

DECLARE_DELEGATE_OneParam(FOnArticyRefChanged, const FArticyRef&);

/**
 * SArticyRefProperty is a widget that displays and manages Articy references.
 */
class ARTICYEDITOR_API SArticyRefProperty : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SArticyRefProperty)
		: _ArticyRefToDisplay(FArticyRef())
		, _TopLevelClassRestriction(UArticyObject::StaticClass())
		, _bExactClass(false)
		, _bExactClassEditable(true)
		, _bIsReadOnly(false)
		{}
		SLATE_ATTRIBUTE(FArticyRef, ArticyRefToDisplay)
		SLATE_EVENT(FOnArticyRefChanged, OnArticyRefChanged)
		SLATE_ATTRIBUTE(UClass*, TopLevelClassRestriction)
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
	 * Updates the widget state on each tick.
	 *
	 * @param AllottedGeometry The geometry allocated for this widget.
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time elapsed since the last tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/**
	 * Creates internal widgets used in the ArticyRefProperty.
	 */
	void CreateInternalWidgets();

private:
	TAttribute<FArticyRef> ArticyRefToDisplay;
	FOnArticyRefChanged OnArticyRefChanged;
	TAttribute<UClass*> TopLevelClassRestriction;
	TAttribute<bool> bExactClass;
	TAttribute<bool> bExactClassEditable;
	TAttribute<bool> bClassFilterEditable;
	TAttribute<bool> bIsReadOnly;

	/**
	 * Called when the Articy ID changes.
	 *
	 * @param ArticyId The new Articy ID.
	 */
	void OnArticyIdChanged(const FArticyId& ArticyId);

	/**
	 * Retrieves the Articy ID to be displayed.
	 *
	 * @return The Articy ID to display.
	 */
	FArticyId GetArticyIdToDisplay() const;

	// the articy object this widget currently represents
	TWeakObjectPtr<UArticyObject> CachedArticyObject = nullptr;
	mutable FArticyRef CachedArticyRef = FArticyRef();

	TSharedPtr<SArticyIdProperty> ArticyIdProperty;
	TSharedPtr<FExtender> ArticyIdExtender;

private:
	/**
	 * Creates additional reference widgets for the toolbar.
	 *
	 * @param Builder The toolbar builder to add widgets to.
	 */
	void CreateAdditionalRefWidgets(FToolBarBuilder& Builder);

	/**
	 * Updates the internal values and fires delegates.
	 *
	 * @param NewRef The new ArticyRef to update.
	 */
	void Update(const FArticyRef& NewRef);

	/**
	 * Updates the widget display and customizations.
	 */
	void UpdateWidget();

private:
	/**
	 * Copies the property value to the clipboard.
	 */
	void OnCopyProperty() const;

	/**
	 * Pastes the property value from the clipboard.
	 */
	void OnPasteProperty();

	/**
	 * Determines whether the property can be pasted.
	 *
	 * @return true if the property can be pasted, false otherwise.
	 */
	bool CanPasteProperty() const;
};
