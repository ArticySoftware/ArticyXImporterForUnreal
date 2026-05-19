//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Customizations/ArticyIdPropertyWidgetCustomizations/DefaultArticyIdPropertyWidgetCustomizations.h"
#include "ArticyEditorStyle.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "ArticyObject.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

/**
 * @brief Registers a custom widget for Articy ID property with additional toolbar button.
 *
 * This method creates an extra button in the toolbar to interact with Articy objects.
 *
 * @param Builder A builder object for constructing Articy ID property widget customizations.
 */
void FArticyButtonCustomization::RegisterArticyIdPropertyWidgetCustomization(FArticyIdPropertyWidgetCustomizationBuilder& Builder)
{
	ArticyObject = Builder.GetArticyObject();

	FArticyIdPropertyWidgetCustomizationInfo Info;
	Info.ExtraButtonExtender = MakeShared<FExtender>();
	Info.ExtraButtonExtender->AddToolBarExtension(TEXT("Base"), EExtensionHook::After, nullptr,
		FToolBarExtensionDelegate::CreateRaw(this, &FArticyButtonCustomization::CreateArticyButton));

	Builder.AddCustomization(Info);
}

/**
 * @brief Unregisters the custom widget for Articy ID property.
 *
 * This method clears the reference to the Articy object when the customization is unregistered.
 */
void FArticyButtonCustomization::UnregisterArticyIdPropertyWidgetCustomization()
{
	ArticyObject = nullptr;
}

/**
 * @brief Creates a button for the Articy toolbar.
 *
 * This method adds a button to the toolbar, allowing users to interact with Articy objects.
 *
 * @param Builder A toolbar builder for constructing toolbar elements.
 */
void FArticyButtonCustomization::CreateArticyButton(FToolBarBuilder& Builder)
{
	const FSlateBrush* ArticyDraftLogo = FArticyEditorStyle::Get().GetBrush("ArticyImporter.ArticyDraft.16");

	TSharedRef<SButton> ArticyButton = SNew(SButton)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this, &FArticyButtonCustomization::OnArticyButtonClicked))
		.ToolTipText(FText::FromString("Show selected object in articy:draft"))
		.Content()
		[
			SNew(SImage)
				.Image(ArticyDraftLogo)
		];

	Builder.AddWidget(ArticyButton);
}

/**
 * @brief Handles the click event for the Articy button.
 *
 * This method is called when the Articy button is clicked, and it shows the selected object in Articy Draft.
 *
 * @return A reply indicating that the button click was handled.
 */
FReply FArticyButtonCustomization::OnArticyButtonClicked()
{
	UserInterfaceHelperFunctions::ShowObjectInArticy(ArticyObject.Get());
	return FReply::Handled();
}

/**
 * @brief Creates an instance of the Articy button customization.
 *
 * This factory method creates and returns a new instance of the Articy button customization.
 *
 * @return A shared pointer to the created customization instance.
 */
TSharedPtr<IArticyIdPropertyWidgetCustomization> FArticyButtonCustomizationFactory::CreateCustomization()
{
	return MakeShareable(new FArticyButtonCustomization);
}

/**
 * @brief Checks if the customization supports the given Articy object type.
 *
 * This method determines whether the customization can be applied to the specified Articy object type.
 *
 * @param ArticyObject The Articy object to check.
 * @return True if the customization supports the object type, otherwise false.
 */
bool FArticyButtonCustomizationFactory::SupportsType(const UArticyObject* ArticyObject)
{
	if (!ArticyObject)
	{
		return false;
	}

	// Supports all Articy objects, hence return true. Modify this in your own override for more advanced customization
	return true;
}
