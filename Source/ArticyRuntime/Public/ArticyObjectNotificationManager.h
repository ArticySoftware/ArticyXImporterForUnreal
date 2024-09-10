//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyChangedProperty.h"
#include "ArticyObjectNotificationManager.generated.h"

/**
 * Function pointer type for handling changes in Articy properties.
 * @param ChangedProperty The property that has changed.
 */
using FArticyPropertyChangedFunction = void(*)(FArticyChangedProperty);

/**
 * Enum defining the types of Articy properties to include in notifications.
 */
enum class EArticyTypeProperties : uint8 {
    IncludeBaseType = 1,      ///< Includes base type properties.
    Template = 2,            ///< Includes template properties.
    General = 4,             ///< Includes general properties.
    ArticyObject = 8,        ///< Includes Articy object-specific properties.
    All = 15                 ///< Includes all types of properties.
};

/**
 * Manager class for handling notifications about changes in Articy objects.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyObjectNotificationManager : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Gets the singleton instance of the notification manager.
     * @return Pointer to the singleton instance.
     */
    static UArticyObjectNotificationManager* Get();

    /**
     * Adds a listener for changes to Articy objects matching the given filter.
     * @param Filter The filter string for identifying Articy objects.
     * @param ChangedFunction The function to call when a property changes.
     */
    void AddListener(const FString& Filter, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * Adds a listener for changes to Articy objects matching the given filter and property flags.
     * @param Filter The filter string for identifying Articy objects.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     */
    void AddListener(const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * Adds a listener for changes to a specific Articy object.
     * @param Object The Articy object to listen to.
     * @param ChangedFunction The function to call when a property changes.
     */
    void AddListener(UArticyBaseObject* Object, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * Removes listeners for Articy objects matching the given filter.
     * @param Filter The filter string for identifying Articy objects.
     */
    void RemoveListeners(const FString& Filter);

    /**
     * Removes listeners from a specific Articy object.
     * @param Object The Articy object to remove listeners from.
     */
    void RemoveListeners(UArticyBaseObject* Object);

protected:
    /**
     * Splits a string into an object name and instance number.
     * @param InString The input string to split.
     * @param OutName The resulting object name.
     * @param OutInstanceNumber The resulting instance number.
     */
    void SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber);
};
