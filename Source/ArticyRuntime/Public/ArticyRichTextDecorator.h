//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateTypes.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/ITextDecorator.h"
#include "Components/RichTextBlockDecorator.h"
#include "Engine/DataTable.h"
#include "ArticyRichTextDecorator.generated.h"

class UArticyObject;
class IArticyHyperlinkHandler;

/**
 * Decorator for rich text blocks that handles Articy-specific text decoration and hyperlink functionality.
 */
UCLASS(Blueprintable)
class ARTICYRUNTIME_API UArticyRichTextDecorator : public URichTextBlockDecorator
{
    GENERATED_BODY()

public:
    /**
     * Constructor for the Articy rich text decorator.
     *
     * @param ObjectInitializer Object initializer for setting up the decorator.
     */
    UArticyRichTextDecorator(const FObjectInitializer& ObjectInitializer);

    /**
     * Creates a new text decorator instance for a rich text block.
     *
     * @param InOwner The rich text block to decorate.
     * @return A shared pointer to the newly created text decorator.
     */
    virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

    /**
     * Callback when a link is navigated.
     *
     * @param Parent The parent rich text block.
     * @param Link The link that was navigated to.
     */
    void OnArticyLinkNavigated(URichTextBlock* Parent, const FString Link);

    /**
     * Finds the parent widget that implements the Articy hyperlink handler interface.
     *
     * @param RichTextBlock The rich text block to search from.
     * @return The widget implementing the hyperlink handler interface, or nullptr if not found.
     */
    UObject* GetHyperlinkHandler(URichTextBlock* RichTextBlock);

    /**
     * Resolves a hyperlink URL into an Articy object.
     *
     * @param Owner The owning rich text block.
     * @param Link The URL to resolve.
     * @return The Articy object referenced by the URL, or nullptr if not found.
     */
    UArticyObject* GetLinkDestination(URichTextBlock* Owner, const FString& Link);

    /** Hyperlink style */
    UPROPERTY(EditAnywhere, Category = "articy")
    FHyperlinkStyle HyperlinkStyle;
};
