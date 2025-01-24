//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "ArticyTexts.generated.h"

/**
 * Struct representing a text definition in Articy.
 */
USTRUCT()
struct ARTICYEDITOR_API FArticyTextDef
{
    GENERATED_BODY()

    /** The text content */
    UPROPERTY(VisibleAnywhere, Category = "Text")
    FString Text = "";

    /** The associated VO (Voice Over) asset */
    UPROPERTY(VisibleAnywhere, Category = "Text")
    FString VoAsset = "";

    /**
     * Imports text definition data from a JSON value.
     *
     * @param Json A shared pointer to the JSON value containing the text definition.
     */
    void ImportFromJson(const TSharedPtr<FJsonValue>& Json);
};

/**
 * Struct representing a collection of texts in Articy.
 */
USTRUCT()
struct FArticyTexts
{
    GENERATED_BODY()

    /** A map of text names to their definitions */
    UPROPERTY(VisibleAnywhere, Category = "Texts")
    TMap<FString, FArticyTextDef> Content;

    /** The context of the texts */
    UPROPERTY(VisibleAnywhere, Category = "Texts")
    FString Context = "";

    /**
     * Imports text data from a JSON object.
     *
     * @param Json A shared pointer to the JSON object containing the text data.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject>& Json);
};
