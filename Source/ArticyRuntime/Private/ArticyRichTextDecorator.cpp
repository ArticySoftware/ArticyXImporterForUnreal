//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyRichTextDecorator.h"
#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Components/PanelWidget.h"
#include "Widgets/Input/SHyperlink.h"
#include "Interfaces/ArticyHyperlinkHandler.h"
#include "Internationalization/Regex.h"
#include "ArticyDatabase.h"

class FArticyRichTextDecorator : public FRichTextDecorator
{
public:
	FArticyRichTextDecorator(URichTextBlock* InOwner, UArticyRichTextDecorator* InDecorator)
		: FRichTextDecorator(InOwner), Decorator(InDecorator)
	{
		// Find the TextStyleSet property in the rich text block class
		for (TFieldIterator<FProperty> It(URichTextBlock::StaticClass()); It; ++It)
		{
			if (It->GetNameCPP() == TEXT("TextStyleSet"))
			{
				DataTableProp = *It;
			}
		}
	}

	/**
	 * Determines if this decorator supports the given text run based on its metadata.
	 *
	 * @param RunParseResult The parsed results of the text run.
	 * @param Text The text of the run.
	 * @return True if the decorator supports the run (i.e., has a color or link attribute), otherwise false.
	 */
	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		// Always true if color appears as an attribute
		if (RunParseResult.MetaData.Contains(TEXT("color")))
			return true;

		// Always true if link appears as an attribute
		if (RunParseResult.MetaData.Contains(TEXT("link")))
			return true;

		return false;
	}

protected:
#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	/**
	 * Creates and applies text decoration based on the run info.
	 *
	 * @param RunInfo Information about the text run.
	 * @param InOutTextStyle The style to apply to the text.
	 * @param InOutString The string being constructed.
	 */
	virtual void CreateDecoratorText(const FTextRunInfo& RunInfo, FTextBlockStyle& InOutTextStyle, FString& InOutString) const override
	{
		// Add text to string
		InOutString += RunInfo.Content.ToString();

		// Check if this style is defined in our table
		UDataTable* StyleTable = GetStyleTable();
		if (StyleTable)
		{
			// If it is, use its text styling by copying the row in
			const FRichTextStyleRow* row = StyleTable->FindRow<FRichTextStyleRow>(*RunInfo.Name, "ArticyStyleTableCheck");
			if (row)
			{
				InOutTextStyle = row->TextStyle;
			}
		}

		// If we have a color attribute
		if (RunInfo.MetaData.Contains(TEXT("color"))) {
			// Change the style color
			const FString color = RunInfo.MetaData[TEXT("color")];
			InOutTextStyle.ColorAndOpacity = FSlateColor(FColor::FromHex(color));
		}
	}
#endif

	/**
	 * Creates a widget for text decoration based on the run info.
	 *
	 * @param RunInfo Information about the text run.
	 * @param TextStyle The style applied to the text.
	 * @return A shared pointer to the created widget, or nullptr if no widget was created.
	 */
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& TextStyle) const override
	{
		// Get link destination
		const FString* Reference = RunInfo.MetaData.Find(TEXT("link"));

		// If this isn't a link, don't use this behavior 
		if (Reference == nullptr) { return nullptr; }

		// Create delegate
		FSimpleDelegate onNavigate;
		onNavigate.BindUObject(Decorator, &UArticyRichTextDecorator::OnArticyLinkNavigated, Owner, *Reference);

		// Create hyperlink with appropriate style
		return SNew(SHyperlink)
			.Text(RunInfo.Content)
			.OnNavigate(onNavigate)
			.Style(&Decorator->HyperlinkStyle);
	}

private:
	// Cached pointer to data table property in URichTextBlock. Needed because the property is protected :(
	FProperty* DataTableProp = nullptr;

	// Pointer to our parent decorator
	UArticyRichTextDecorator* Decorator = nullptr;

	/**
	 * Gets the style table from the owning rich text block.
	 *
	 * @return A pointer to the style table, or nullptr if the table could not be retrieved.
	 */
	UDataTable* GetStyleTable() const
	{
		if (DataTableProp == nullptr) {
			return nullptr;
		}
		return *DataTableProp->ContainerPtrToValuePtr<UDataTable*>(Owner, 0);
	}
};

UArticyRichTextDecorator::UArticyRichTextDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/**
 * Creates a new instance of the text decorator for the specified rich text block.
 *
 * @param InOwner The rich text block to decorate.
 * @return A shared pointer to the newly created text decorator.
 */
TSharedPtr<ITextDecorator> UArticyRichTextDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FArticyRichTextDecorator(InOwner, this));
}

/**
 * Finds the parent widget that implements the Articy hyperlink handler interface.
 *
 * @param RichTextBlock The rich text block to search from.
 * @return The widget implementing the hyperlink handler interface, or nullptr if not found.
 */
UObject* UArticyRichTextDecorator::GetHyperlinkHandler(URichTextBlock* RichTextBlock)
{
	UWidget* Widget = RichTextBlock;
	while (Widget != nullptr && !Widget->GetClass()->ImplementsInterface(UArticyHyperlinkHandler::StaticClass()))
	{
		UWidget* Next = Widget->GetParent();
		if (Next == nullptr)
		{
			UWidgetTree* Tree = Cast<UWidgetTree>(Widget->GetOuter());
			if (Tree)
			{
				Next = Cast<UWidget>(Tree->GetOuter());
			}
		}
		Widget = Next;
	}

	// No widget in the hierarchy implements the handler interface.
	if (Widget == nullptr) { return nullptr; }

	// Return interface
	return Widget;
}

/**
 * Resolves the link text into an Articy object based on the provided URL.
 *
 * @param Owner The owning rich text block.
 * @param Link The URL to resolve.
 * @return The Articy object referenced by the URL, or nullptr if not found.
 */
UArticyObject* UArticyRichTextDecorator::GetLinkDestination(URichTextBlock* Owner, const FString& Link)
{
	static FRegexPattern Pattern(TEXT("articy:\\/\\/localhost\\/view\\/~\\/(\\d+)"));
	FRegexMatcher myMatcher(Pattern, Link);

	// If the link doesn't match the expected format, abort.
	if (!myMatcher.FindNext()) { return nullptr; }

	// Get numeric id
	FString Id = myMatcher.GetCaptureGroup(1);
	uint64 id = FCString::Strtoui64(*Id, nullptr, 10);

	// Resolve
	return UArticyDatabase::Get(Owner)->GetObject(id);
}

/**
 * Handles the navigation of an Articy link by resolving the link and notifying the hyperlink handler.
 *
 * @param Parent The parent rich text block.
 * @param Link The link that was navigated to.
 */
void UArticyRichTextDecorator::OnArticyLinkNavigated(URichTextBlock* Parent, const FString Link)
{
	auto Widget = GetHyperlinkHandler(Parent);
	if (Widget == nullptr) { return; }

	// Resolve the link text into an articy object
	UArticyObject* Object = GetLinkDestination(Parent, Link);
	if (!Object) { return; }

	// Call handler
	IArticyHyperlinkHandler::Execute_OnHyperlinkNavigated(Widget, Object, Parent);
}
