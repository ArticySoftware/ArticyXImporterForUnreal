//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Slate/AssetPicker/SArticyObjectToolTip.h"
#include "ArticyFlowClasses.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithText.h"
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>
#include <EditorStyleSet.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/SToolTip.h>
#include <Internationalization/Text.h>
#include "Slate/UserInterfaceHelperFunctions.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "ArticyEditorModule.h"

#define LOCTEXT_NAMESPACE "ArticyObjectToolTip"

void SArticyObjectToolTip::Construct(const FArguments& InArgs)
{
	ArticyIdAttribute = InArgs._ObjectToDisplay;

	TooltipBrush.ImageSize = FVector2D(64.f, 64.f);

	SToolTip::Construct(
		SToolTip::FArguments()
		.TextMargin(1.f)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		.BorderImage(FAppStyle::Get().GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
#else
		.BorderImage(FEditorStyle::Get().GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
#endif
		// Text makes tooltip show, probably because it doesn't initialize otherwise
		.Text(FText::FromString("TEST"))
	);
}

void SArticyObjectToolTip::OnOpening()
{
	if (CachedArticyObject.IsValid())
	{
		SetContentWidget(CreateToolTipContent());
	}
	else
	{
		SetContentWidget(CreateContentForEmpty());
	}

	UpdateWidget();
}

void SArticyObjectToolTip::OnClosed()
{
	SetContentWidget(SNullWidget::NullWidget);
}

TSharedRef<SWidget> SArticyObjectToolTip::CreateTooltipWidget(FText NameText, TSharedRef<SVerticalBox> InfoBox)
{
	TSharedRef<SVerticalBox> OverallTooltipVBox = SNew(SVerticalBox);

	// Top section (asset name, type, is checked out)
	OverallTooltipVBox->AddSlot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(SBorder)
				.Padding(6)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#endif
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.Padding(0, 0, 4, 0)
								[
									SNew(STextBlock)
										.Text(NameText)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0					
										.Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
#else
										.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
#endif
										.AutoWrapText(true)
								]
						]
				]
		];

	// Bottom section (additional information)
	OverallTooltipVBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
				.Padding(6)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#endif
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							InfoBox
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Top)
						.AutoWidth()
						[
							SNew(SBox)
								.Padding(FMargin(10, 2, 2, 2))
								[
									SNew(SImage)
										.Image(this, &SArticyObjectToolTip::GetTooltipImage)
								]
						]
				]
		];

	return SNew(SBorder)
		.Padding(6)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0	
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
#endif
		[
			SNew(SBox)
				.MaxDesiredWidth(500.f)
				[
					OverallTooltipVBox
				]
		];
}

TSharedRef<SWidget> SArticyObjectToolTip::CreateToolTipContent()
{
	// use the preview image if available
	const bool bHasPreviewImage = UserInterfaceHelperFunctions::RetrievePreviewImage(CachedArticyObject.Get(), TooltipBrush);

	// if there is no preview image, use the preview image of the speaker, if available
	if (!bHasPreviewImage)
	{
		const bool bHasSpeakerPreviewImage = UserInterfaceHelperFunctions::RetrieveSpeakerPreviewImage(CachedArticyObject.Get(), TooltipBrush);

		// if there is no speaker preview image, use the type image instead
		if (!bHasSpeakerPreviewImage)
		{
			TooltipBrush = *UserInterfaceHelperFunctions::GetArticyTypeImage(CachedArticyObject.Get(), UserInterfaceHelperFunctions::Large);
		}
	}

	const FString AssetName = CachedArticyObject.Get()->GetName();
	const UClass* ClassOfObject = CachedArticyObject.Get()->UObject::GetClass();

	// The tooltip contains the name, class, path, and asset registry tags
	// Use asset name by default, overwrite with display name where it makes sense
	FText NameText = FText::FromString(AssetName);
	const FText ClassText = FText::Format(LOCTEXT("ClassName", "({0})"), FText::FromString(ClassOfObject->GetName()));

	// Create a box to hold every line of info in the body of the tooltip
	TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

	// overwrite the asset name with the display name
	bool bUsingDisplayName = false;
	IArticyObjectWithDisplayName* ArticyObjectWithDisplayName = Cast<IArticyObjectWithDisplayName>(CachedArticyObject);
	if (ArticyObjectWithDisplayName)
	{
		const FText DisplayName = ArticyObjectWithDisplayName->GetDisplayName();
		if (!DisplayName.IsEmpty())
		{
			NameText = DisplayName;
			bUsingDisplayName = true;
		}
	}

	IArticyObjectWithSpeaker* ArticyObjectWithSpeaker = Cast<IArticyObjectWithSpeaker>(CachedArticyObject);
	if (ArticyObjectWithSpeaker)
	{
		const UArticyObject* Speaker = UArticyObject::FindAsset(ArticyObjectWithSpeaker->GetSpeakerId());
		// Speaker can be nullptr in case a speaker that does not exist as an entity was specified, i.e. in the scriptwriting documents
		if (Speaker)
		{
			const IArticyObjectWithDisplayName* DisplayNameOfSpeaker = Cast<IArticyObjectWithDisplayName>(Speaker);
			const FText DisplayName = DisplayNameOfSpeaker->GetDisplayName();
			AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyObjectToolTipSpeaker", "Speaker"),
				DisplayName, true);
		}
		else
		{
			UE_LOG(LogArticyEditor, Error, TEXT("Articy tooltip: Speaker object does not exist"))
		}

	}

	// add the text to the tooltip body if possible
	IArticyObjectWithText* ArticyObjectWithText = Cast<IArticyObjectWithText>(CachedArticyObject);
	if (ArticyObjectWithText)
	{
		const FText& Text = ArticyObjectWithText->GetText();
		if (Text.IsEmpty())
		{
			// if there is empty text, add no text at all - maybe "..." or "Empty"?
		}
		else
		{
			AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyObjectToolTipText", "Text"), FText::FromString(FString("\"").Append(Text.ToString()).Append("\"")), true);
		}
	}


	// if we overwrote the asset name with the display name, attach the asset name in the tooltip body
	if (bUsingDisplayName)
	{
		AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyObjectToolTipAssetName", "Asset Name"), FText::FromString(AssetName), false);
	}

	// if our object has a target, add the display name of the target to the tooltip
	const FArticyId* TargetID = UserInterfaceHelperFunctions::GetTargetID(CachedArticyObject.Get());
	if (TargetID)
	{
		UArticyObject* TargetObject = UArticyObject::FindAsset(*TargetID);
		if (TargetObject)
		{
			AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyObjectToolTipTarget", "Target"), FText::FromString(UserInterfaceHelperFunctions::GetDisplayName(TargetObject)), false);
		}
	}

	// add class name
	AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyObjectToolTipClass", "Class"), ClassText, false);

	const FText ArticyIdText = FText::FromString(ArticyIdAttribute.Get().ToString());
	// add id
	AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyId", "Id"), ArticyIdText, true);

	return CreateTooltipWidget(NameText, InfoBox);
}

TSharedRef<SWidget> SArticyObjectToolTip::CreateContentForEmpty()
{
	check(!CachedArticyObject.IsValid());

	// will result in the "empty" image
	TooltipBrush = *UserInterfaceHelperFunctions::GetArticyTypeImage(nullptr, UserInterfaceHelperFunctions::Large);

	TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

	const FText ArticyIdText = FText::FromString(ArticyIdAttribute.Get().ToString());
	AddToToolTipInfoBox(InfoBox, LOCTEXT("ArticyId", "Id"), ArticyIdText, true);

	FText ObjectNotFoundText = FText::FromString("None");

	if (!ArticyIdAttribute.Get().IsNull())
	{
		const FString ObjectNotFoundString = TEXT("Articy Object not found");
		ObjectNotFoundText = FText::FromString(ObjectNotFoundString);
	}

	return CreateTooltipWidget(ObjectNotFoundText, InfoBox);
}

void SArticyObjectToolTip::AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value, bool bImportant) const
{
	FWidgetStyle ImportantStyle;
	ImportantStyle.SetForegroundColor(FLinearColor(1, 0.5, 0, 1));

	InfoBox->AddSlot()
		.AutoHeight()
		.Padding(0, 1)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("AssetViewTooltipFormat", "{0}:"), Key))
						.ColorAndOpacity(bImportant ? ImportantStyle.GetSubduedForegroundColor() : FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					// highlighting text reference: AssetViewWidgets.cpp 1118
					SNew(STextBlock).Text(Value)
						.WrapTextAt(400)
						.ColorAndOpacity(bImportant ? ImportantStyle.GetForegroundColor() : FSlateColor::UseSubduedForeground())
						.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
				]
		];
}

void SArticyObjectToolTip::UpdateWidget()
{
	CachedArticyId = ArticyIdAttribute.Get();
	CachedArticyObject = UArticyObject::FindAsset(CachedArticyId);

	if (CachedArticyObject.IsValid())
	{
		SetContentWidget(CreateToolTipContent());
	}
	else
	{
		SetContentWidget(CreateContentForEmpty());
	}
}

const FSlateBrush* SArticyObjectToolTip::GetTooltipImage() const
{
	return &TooltipBrush;
}

#undef LOCTEXT_NAMESPACE
