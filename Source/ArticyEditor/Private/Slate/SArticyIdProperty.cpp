//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/SArticyIdProperty.h"
#include <Templates/SharedPointer.h>
#include <Kismet2/KismetEditorUtilities.h>
#include <Kismet2/SClassPickerDialog.h>
#include <IContentBrowserSingleton.h>
#include <ContentBrowserModule.h>
#include <Widgets/Input/SComboButton.h>
#include "ArticyObject.h"
#include "ArticyEditorModule.h"
#include "Slate/AssetPicker/SArticyObjectAssetPicker.h"
#include "Editor.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "ArticyRefProperty"

/**
 * Ticks the SFixedSizeMenuComboButton, updating its position if necessary.
 *
 * @param AllottedGeometry The allotted geometry of the widget.
 * @param InCurrentTime The current time.
 * @param InDeltaTime The time delta since the last tick.
 */
void SFixedSizeMenuComboButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	TSharedPtr<SWindow> PopupWindow = PopupWindowPtr.Pin();
	if (PopupWindow.IsValid() && IsOpenViaCreatedWindow())
	{
		// Figure out where our attached pop-up window should be placed.
		const FVector2D PopupContentDesiredSize = PopupWindow->GetContent()->GetDesiredSize();
		//const FVector2D PopupContentDesiredSize = PopupWindow->GetContent()->GetDesiredSize();
		FGeometry PopupGeometry = ComputeNewWindowMenuPlacement(AllottedGeometry, PopupContentDesiredSize, Placement.Get());
		const FVector2D NewPosition = PopupGeometry.LocalToAbsolute(FVector2D::ZeroVector);

		// We made a window for showing the popup.
		// Update the window's position!
		PopupWindow->MoveWindowTo(NewPosition);
	}

	/** The tick is ending, so the window was not dismissed this tick. */
	bDismissedThisTick = false;
}

/**
 * Constructs the SArticyIdProperty widget.
 *
 * @param InArgs The arguments for widget construction.
 */
void SArticyIdProperty::Construct(const FArguments& InArgs)
{
	if (ArticyIdToDisplay.IsBound())
	{
		ensureMsgf(InArgs._OnArticyIdChanged.IsBound(), TEXT("Since the shown object is given externally per event, the handler also needs to handle updates."));
	}

	this->ArticyIdToDisplay = InArgs._ArticyIdToDisplay;
	this->OnArticyIdChanged = InArgs._OnArticyIdChanged;
	this->TopLevelClassRestriction = InArgs._TopLevelClassRestriction;
	this->CustomizationHighExtender = InArgs._HighExtender;
	this->bExactClass = InArgs._bExactClass;
	this->bExactClassEditable = InArgs._bExactClassEditable;
	this->CopyAction = InArgs._CopyAction;
	this->PasteAction = InArgs._PasteAction;
	this->bIsReadOnly = InArgs._bIsReadOnly;

	SetCursor(EMouseCursor::Hand);

	CachedArticyId = ArticyIdToDisplay.Get(FArticyId());
	CachedArticyObject = !CachedArticyId.IsNull() ? UArticyObject::FindAsset(CachedArticyId) : nullptr;

	CreateInternalWidgets();

	UpdateWidget();

	this->ChildSlot
		[
			ChildBox.ToSharedRef()
		];
}

/**
 * Ticks the SArticyIdProperty widget, updating it if necessary.
 *
 * @param AllottedGeometry The allotted geometry of the widget.
 * @param InCurrentTime The current time.
 * @param InDeltaTime The time delta since the last tick.
 */
void SArticyIdProperty::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	const FArticyId CurrentRefId = ArticyIdToDisplay.Get() ? ArticyIdToDisplay.Get() : FArticyId();
	if (CurrentRefId != CachedArticyId || (!CurrentRefId.IsNull() && !CachedArticyObject.IsValid()))
	{
		Update(CurrentRefId);
	}
}

/**
 * Creates the internal widgets for the SArticyIdProperty.
 */
void SArticyIdProperty::CreateInternalWidgets()
{
	ComboButton = SNew(SFixedSizeMenuComboButton)
#if __cplusplus >= 202002L
		.IsEnabled_Lambda([=, this]()
#else
			.IsEnabled_Lambda([=]()
#endif
				{
					return !bIsReadOnly.Get();
				})
			.OnGetMenuContent(this, &SArticyIdProperty::CreateArticyObjectAssetPicker)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(this, &SArticyIdProperty::OnGetArticyObjectDisplayName)
					];

				if (!CopyAction.IsBound())
				{
					CopyAction.ExecuteAction = FExecuteAction::CreateSP(this, &SArticyIdProperty::OnCopyProperty);
				}

				if (!PasteAction.IsBound())
				{
					PasteAction.CanExecuteAction = FCanExecuteAction::CreateSP(this, &SArticyIdProperty::CanPasteProperty);
					PasteAction.ExecuteAction = FExecuteAction::CreateSP(this, &SArticyIdProperty::OnPasteProperty);
				}

				TileView = SNew(SArticyObjectTileView)
					.ArticyIdToDisplay(ArticyIdToDisplay)
					.OnArticyIdChanged(OnArticyIdChanged)
					.CopyAction(CopyAction)
					.PasteAction(PasteAction)
					.OnMouseDoubleClick(this, &SArticyIdProperty::OnAssetThumbnailDoubleClick)
					.ThumbnailSize(ArticyRefPropertyConstants::ThumbnailSize)
					.ThumbnailPadding(ArticyRefPropertyConstants::ThumbnailPadding)
					.bIsReadOnly(bIsReadOnly);

				CustomizationButtonBox_High = SNew(SHorizontalBox);
				if (CustomizationHighExtender.IsValid())
				{
					FToolBarBuilder Builder(nullptr, FMultiBoxCustomization("ArticyIdCustomizationHigh"), CustomizationHighExtender);

					Builder.BeginSection("Base");
					Builder.EndSection();

					CustomizationButtonBox_High->AddSlot()
						[
							Builder.MakeWidget()
						];
				}

				CustomizationButtonBox_Low = SNew(SHorizontalBox);

				ChildBox = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(0, 0, 2, 0)
					[
						SAssignNew(ThumbnailBorder, SBorder)
							.Padding(5.0f)
							[
								SAssignNew(TileContainer, SBox)
									[
										TileView.ToSharedRef()
									]
							]
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(3, 0, 3, 0)
							[
								CustomizationButtonBox_High.ToSharedRef()
							]
							+ SVerticalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(3, 1, 3, 1)
							.AutoHeight()
							[
								ComboButton.ToSharedRef()
							]
							+ SVerticalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(3, 0, 3, 0)
							.AutoHeight()
							[
								SNew(SBox)
									.MinDesiredHeight(32.f)
									[
										CustomizationButtonBox_Low.ToSharedRef()
									]
							]
					];
}

/**
 * Updates the SArticyIdProperty widget with a new Articy ID.
 *
 * @param NewId The new Articy ID to update the widget with.
 */
void SArticyIdProperty::Update(const FArticyId& NewId)
{
	// the actual update. This will be forwarded into the tile view and will cause an update
	CachedArticyId = NewId;
	CachedArticyObject = !CachedArticyId.IsNull() ? UArticyObject::FindAsset(CachedArticyId) : nullptr;

	UpdateWidget();
}

/**
 * Updates the SArticyIdProperty widget, including any customizations.
 */
void SArticyIdProperty::UpdateWidget()
{
	for (TSharedPtr<IArticyIdPropertyWidgetCustomization>& Customization : ActiveCustomizations)
	{
		Customization->UnregisterArticyIdPropertyWidgetCustomization();
	}
	ActiveCustomizations.Reset();

	FArticyEditorModule::Get().GetCustomizationManager()->CreateArticyIdPropertyWidgetCustomizations(CachedArticyObject.Get(), ActiveCustomizations);

	FArticyIdPropertyWidgetCustomizationBuilder Builder(CachedArticyObject.Get());
	for (TSharedPtr<IArticyIdPropertyWidgetCustomization>& Customization : ActiveCustomizations)
	{
		Customization->RegisterArticyIdPropertyWidgetCustomization(Builder);
	}

	// this empties the extra buttons box and refills it based on the registered customizations
	ApplyArticyRefCustomizations(Builder.GetCustomizations());
}

/**
 * Applies a single ArticyRef customization.
 *
 * @param Customization The customization to apply.
 */
void SArticyIdProperty::ApplyArticyRefCustomization(const FArticyIdPropertyWidgetCustomizationInfo& Customization)
{
	if (Customization.ExtraButtonExtender.IsValid())
	{
		ArticyIdCustomizationExtenders.Add(Customization.ExtraButtonExtender);
	}
}

/**
 * Applies a series of ArticyRef customizations.
 *
 * @param Customizations The array of customizations to apply.
 */
void SArticyIdProperty::ApplyArticyRefCustomizations(const TArray<FArticyIdPropertyWidgetCustomizationInfo>& Customizations)
{
	ArticyIdCustomizationExtenders.Empty();
	CustomizationButtonBox_Low->ClearChildren();

	for (const FArticyIdPropertyWidgetCustomizationInfo& Info : Customizations)
	{
		ApplyArticyRefCustomization(Info);
	}

	TSharedPtr<FExtender> ExtraButtonExtender = FExtender::Combine(ArticyIdCustomizationExtenders);
	FToolBarBuilder Builder(nullptr, FMultiBoxCustomization(TEXT("ArticyIdCustomizationLow")), ExtraButtonExtender);

	// we need to begin a section so the extenders know where to apply themselves
	Builder.BeginSection(TEXT("Base"));
	Builder.EndSection();

	CustomizationButtonBox_Low->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			Builder.MakeWidget()
		];
}

/**
 * Creates the ArticyObject asset picker widget.
 *
 * @return The created widget.
 */
TSharedRef<SWidget> SArticyIdProperty::CreateArticyObjectAssetPicker()
{
	TSharedRef<SArticyObjectAssetPicker> AssetPicker = SNew(SArticyObjectAssetPicker)
		.OnArticyObjectSelected(this, &SArticyIdProperty::OnArticyObjectPicked)
		.CurrentClassRestriction(TopLevelClassRestriction.Get())
		.TopLevelClassRestriction(TopLevelClassRestriction)
		.bExactClass(bExactClass)
		.bExactClassEditable(bExactClassEditable)
		.bClassFilterEditable(this, &SArticyIdProperty::IsClassFilterEditable);
	return AssetPicker;
}

/**
 * Determines if the class filter is editable.
 *
 * @return true if the class filter is editable, false otherwise.
 */
bool SArticyIdProperty::IsClassFilterEditable() const
{
	return bExactClass.Get() == false || bExactClassEditable.Get() == true;
}

/**
 * Handles the event when an Articy object is picked.
 *
 * @param ArticyObjectData The data of the picked Articy object.
 */
void SArticyIdProperty::OnArticyObjectPicked(const FAssetData& ArticyObjectData) const
{
	UArticyObject* NewObject = Cast<UArticyObject>(ArticyObjectData.GetAsset());

	if (NewObject)
	{
		OnArticyIdChanged.ExecuteIfBound(NewObject->GetId());
	}
	else
	{
		OnArticyIdChanged.ExecuteIfBound(FArticyId());
	}
}

/**
 * Handles the event when the Articy button is clicked.
 *
 * @return The reply result for the button click.
 */
FReply SArticyIdProperty::OnArticyButtonClicked() const
{
	UserInterfaceHelperFunctions::ShowObjectInArticy(UArticyObject::FindAsset(CachedArticyId));
	return FReply::Handled();
}

/**
 * Handles the event when the asset thumbnail is double-clicked.
 *
 * @param InMyGeometry The geometry of the widget.
 * @param InMouseEvent The mouse event data.
 * @return The reply result for the double-click.
 */
FReply SArticyIdProperty::OnAssetThumbnailDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) const
{
	if (CachedArticyObject.IsValid())
	{
		GEditor->EditObject(CachedArticyObject.Get());
	}

	return FReply::Handled();
}

/**
 * Gets the display name of the current Articy object.
 *
 * @return The display name as text.
 */
FText SArticyIdProperty::OnGetArticyObjectDisplayName() const
{
	const FString DisplayName = UserInterfaceHelperFunctions::GetDisplayName(CachedArticyObject.Get());
	return FText::FromString(DisplayName);
}

/**
 * Gets the current Articy object ID.
 *
 * @return The current Articy ID.
 */
FArticyId SArticyIdProperty::GetCurrentObjectID() const
{
	return CachedArticyId;
}

/**
 * Handles the event when the copy property action is triggered.
 */
void SArticyIdProperty::OnCopyProperty() const
{
	FString ValueString = CachedArticyId.ToString();
	FPlatformApplicationMisc::ClipboardCopy(*ValueString);
}

/**
 * Handles the event when the paste property action is triggered.
 */
void SArticyIdProperty::OnPasteProperty()
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	FArticyId NewId = CachedArticyId;
	bool bSuccess = NewId.InitFromString(ClipboardContent);
	if (ensureMsgf(bSuccess, TEXT("String was garbage, therefore Id was not properly updated")))
	{
		OnArticyIdChanged.ExecuteIfBound(NewId);
		Update(NewId);
	}
}

/**
 * Checks if the paste property action can be executed.
 *
 * @return true if the paste action can be executed, false otherwise.
 */
bool SArticyIdProperty::CanPasteProperty() const
{
	if (bIsReadOnly.Get())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	if (ClipboardContent.IsEmpty() || !ClipboardContent.Contains("Low=") || !ClipboardContent.Contains("High="))
	{
		return false;
	}

	FArticyId CandidateId;
	if (CandidateId.InitFromString(ClipboardContent))
	{
		UArticyObject* Object = UArticyObject::FindAsset(CandidateId);
		if (!Object)
		{
			return false;
		}

		return Object->UObject::GetClass()->IsChildOf(TopLevelClassRestriction.Get());
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
