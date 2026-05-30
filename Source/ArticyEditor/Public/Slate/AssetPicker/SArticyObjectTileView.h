// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "ArticyObject.h"
#include "Widgets/Layout/SBorder.h"
#include "ArticyBaseTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Commands/UIAction.h"

DECLARE_DELEGATE_OneParam(FOnArticyIdChanged, const FArticyId&);

/**
 * @brief A widget for displaying Articy objects in a tile view.
 *
 * This widget displays an Articy object with a preview image, type image, and context menu.
 */
class ARTICYEDITOR_API SArticyObjectTileView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArticyObjectTileView)
		: _LabelVisibility(EVisibility::Visible)
		, _OnMouseDoubleClick()
		, _ThumbnailSize(FVector2D(48.f, 48.f))
		, _ThumbnailPadding(FMargin(2.f))
		, _bIsReadOnly(false)
		{}
		SLATE_ATTRIBUTE(FArticyId, ArticyIdToDisplay)
		SLATE_ATTRIBUTE(EVisibility, LabelVisibility)
		SLATE_EVENT(FPointerEventHandler, OnMouseDoubleClick)
		SLATE_EVENT(FOnArticyIdChanged, OnArticyIdChanged)
		SLATE_ARGUMENT(FVector2D, ThumbnailSize)
		SLATE_ARGUMENT(FMargin, ThumbnailPadding)
		SLATE_ATTRIBUTE(bool, bIsReadOnly)
		SLATE_ARGUMENT(FUIAction, CopyAction)
		SLATE_ARGUMENT(FUIAction, PasteAction)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the widget.
	 *
	 * This method initializes the widget with the given arguments.
	 *
	 * @param InArgs The arguments for widget construction.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Ticks the widget for updates.
	 *
	 * This method is called each frame to update the widget.
	 *
	 * @param AllottedGeometry The geometry of the widget.
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time elapsed since the last tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/**
	 * @brief Updates the widget with a new Articy ID.
	 *
	 * This method updates the cached Articy ID and refreshes the widget's display.
	 *
	 * @param NewArticyId The new Articy ID to display.
	 */
	void Update(const FArticyId& NewArticyId);

private:
	TAttribute<FArticyId> ArticyIdToDisplay; //!< The Articy ID to display in the widget.
	FOnArticyIdChanged OnArticyIdChanged; //!< Delegate for handling Articy ID change events.
	FOnArticyIdChanged OnArticyIdPasted; //!< Delegate for handling Articy ID paste events.
	FPointerEventHandler OnMouseDoubleClick; //!< Delegate for handling mouse double-click events.
	TAttribute<EVisibility> LabelVisibility; //!< Attribute for controlling the visibility of the label.
	FVector2D ThumbnailSize; //!< The size of the thumbnail to display.
	FMargin ThumbnailPadding; //!< The padding around the thumbnail.
	TAttribute<bool> bIsReadOnly; //!< Attribute indicating whether the widget is read-only.
	FUIAction CopyAction; //!< Action for copying the Articy ID.
	FUIAction PasteAction; //!< Action for pasting the Articy ID.

	mutable FArticyId CachedArticyId; //!< Cached Articy ID for the widget.
	mutable TWeakObjectPtr<UArticyObject> CachedArticyObject; //!< Cached Articy object pointer.

	TSharedPtr<SImage> PreviewImage; //!< Shared pointer to the preview image widget.
	TSharedPtr<STextBlock> DisplayNameTextBlock; //!< Shared pointer to the display name text block.
	TSharedPtr<SBorder> WidgetContainerBorder; //!< Shared pointer to the widget container border.

	mutable FSlateBrush PreviewBrush; //!< Brush for rendering the preview image.
	const FSlateBrush* TypeImage = nullptr; //!< Pointer to the type image brush.
	TSharedPtr<FTextBlockStyle> EntityNameTextStyle; //!< Shared pointer to the text style for entity names.
	bool bHasPreviewImage = false; //!< Flag indicating whether a preview image is available.

private:
	/**
	 * @brief Opens the context menu for the widget.
	 *
	 * This method constructs the context menu with copy and paste actions if available.
	 *
	 * @param Builder The menu builder for constructing the context menu.
	 */
	void OnContextMenuOpening(class FMenuBuilder& Builder) const;

	/**
	 * @brief Handles mouse button down events.
	 *
	 * This method captures the mouse on right-click to open the context menu.
	 *
	 * @param MyGeometry The geometry of the widget.
	 * @param MouseEvent The mouse event information.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse button up events.
	 *
	 * This method opens the context menu on right-click.
	 *
	 * @param MyGeometry The geometry of the widget.
	 * @param MouseEvent The mouse event information.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Updates the widget's display elements.
	 *
	 * This method refreshes the preview image and type image based on the current Articy object.
	 */
	void UpdateWidget();

	/**
	 * @brief Gets the entity name to display.
	 *
	 * @return The display name of the Articy object.
	 */
	virtual FText OnGetEntityName() const;

	/**
	 * @brief Gets the entity image to display.
	 *
	 * @return The brush to use for the entity image.
	 */
	virtual const FSlateBrush* OnGetEntityImage() const;

	/**
	 * @brief Determines if the preview image is visible.
	 *
	 * @return The visibility state of the preview image.
	 */
	EVisibility OnHasPreviewImage() const;

	/**
	 * @brief Gets the color to use for the Articy object border.
	 *
	 * @return The color for the border.
	 */
	virtual FSlateColor OnGetArticyObjectColor() const;

	/**
	 * @brief Gets the type image to display.
	 *
	 * @return The brush to use for the type image.
	 */
	const FSlateBrush* GetTypeImage() const;
};
