//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include <Widgets/SToolTip.h>
#include "Widgets/SBoxPanel.h"
#include "ArticyObject.h"

// Reference: AssetViewWidgets.h: CreateToolTipWidget

class SArticyObjectTileView;

/**
 * A custom tooltip widget for displaying details about an Articy object.
 */
class SArticyObjectToolTip : public SToolTip
{
public:
	SLATE_BEGIN_ARGS(SArticyObjectToolTip)
		{}
		SLATE_ATTRIBUTE(FArticyId, ObjectToDisplay)
	SLATE_END_ARGS()

	/**
	 * Constructs the tooltip widget.
	 *
	 * @param InArgs The declaration data for this widget.
	 */
	void Construct(const FArguments& InArgs);

	/** Called when the tooltip is opening. */
	virtual void OnOpening() override;

	/** Called when the tooltip is closed. */
	virtual void OnClosed() override;

protected:
	/** Updates the tooltip widget's content. */
	void UpdateWidget();

	/** The attribute containing the Articy ID to display. */
	TAttribute<FArticyId> ArticyIdAttribute;

	/** Cached Articy ID for the current object. */
	mutable FArticyId CachedArticyId;

	/** Cached Articy object pointer. */
	mutable TWeakObjectPtr<UArticyObject> CachedArticyObject;

	/** Brush for displaying the tooltip image. */
	FSlateBrush TooltipBrush;

	/**
	 * Creates the tooltip widget with the specified content.
	 *
	 * @param NameText The name text to display in the tooltip.
	 * @param InfoBox The information box containing additional details.
	 * @return The constructed tooltip widget.
	 */
	TSharedRef<SWidget> CreateTooltipWidget(FText NameText, TSharedRef<SVerticalBox> InfoBox);

	/**
	 * Creates the content for a valid Articy object tooltip.
	 *
	 * @return The constructed content widget.
	 */
	TSharedRef<SWidget> CreateToolTipContent();

	/**
	 * Creates the content for an empty or invalid Articy object tooltip.
	 *
	 * @return The constructed content widget.
	 */
	TSharedRef<SWidget> CreateContentForEmpty();

	/**
	 * Adds key-value type content to the tooltip info box.
	 *
	 * @param InfoBox The info box to add the content to.
	 * @param Key The key text.
	 * @param Value The value text.
	 * @param bImportant Whether the content is considered important.
	 */
	void AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value, bool bImportant) const;

private:
	/**
	 * Gets the tooltip image to display.
	 *
	 * @return The tooltip image brush.
	 */
	const FSlateBrush* GetTooltipImage() const;
};
