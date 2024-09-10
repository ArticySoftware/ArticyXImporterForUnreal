//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyObjectNotificationManager.h"
#include "ArticyBaseObject.h"
#include "ArticyDatabase.h"

/**
 * Gets the singleton instance of the notification manager.
 * Creates a new instance if one does not exist.
 * @return Pointer to the singleton instance.
 */
UArticyObjectNotificationManager* UArticyObjectNotificationManager::Get()
{
    static TWeakObjectPtr<UArticyObjectNotificationManager> ArticyObjectNotificationManager;

    if (!ArticyObjectNotificationManager.IsValid())
    {
        ArticyObjectNotificationManager = TWeakObjectPtr<UArticyObjectNotificationManager>(NewObject<UArticyObjectNotificationManager>());
    }

    return ArticyObjectNotificationManager.Get();
}

/**
 * Splits a string into an object name and instance number based on angle brackets.
 * @param InString The input string to split.
 * @param OutName The resulting object name.
 * @param OutInstanceNumber The resulting instance number.
 */
void UArticyObjectNotificationManager::SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber)
{
    const FString SearchSubstr = TEXT("<");
    const int32 StartIdx = InString.Find(SearchSubstr);

    if (StartIdx != INDEX_NONE)
    {
        const int32 EndIdx = InString.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, StartIdx);

        if (EndIdx != INDEX_NONE)
        {
            OutName = InString.Left(StartIdx);
            OutInstanceNumber = InString.Mid(StartIdx + 1, EndIdx - StartIdx - 1);
        }
        else
        {
            OutName = InString;
            OutInstanceNumber = TEXT("0");
        }
    }
    else
    {
        OutName = InString;
        OutInstanceNumber = TEXT("0");
    }
}

/**
 * Adds a listener for changes to Articy objects matching the given filter.
 * Resolves the object based on the filter and adds the listener.
 * @param Filter The filter string for identifying Articy objects.
 * @param ChangedFunction The function to call when a property changes.
 */
void UArticyObjectNotificationManager::AddListener(const FString& Filter, FArticyPropertyChangedFunction ChangedFunction)
{
    FString ObjectName, ObjectInstance;
    SplitInstance(Filter, ObjectName, ObjectInstance);
    UArticyObject* Object;
    auto DB = UArticyDatabase::Get(this);
    if (Filter.StartsWith(TEXT("0x")))
    {
        Object = DB->GetObject<UArticyObject>(FArticyId{ ArticyHelpers::HexToUint64(ObjectName) }, FCString::Atod(*ObjectInstance));
    }
    else if (Filter.IsNumeric())
    {
        Object = DB->GetObject<UArticyObject>(FArticyId{ FCString::Strtoui64(*ObjectName, nullptr, 10) }, FCString::Atod(*ObjectInstance));
    }
    else
    {
        Object = DB->GetObjectByName(*ObjectName, FCString::Atod(*ObjectInstance));
    }

    if (Object)
    {
        AddListener(Object, ChangedFunction);
    }
}

/**
 * Adds a listener for changes to Articy objects matching the given filter and property flags.
 * @param Filter The filter string for identifying Articy objects.
 * @param Flags The types of properties to include in notifications.
 * @param ChangedFunction The function to call when a property changes.
 */
void UArticyObjectNotificationManager::AddListener(const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction)
{
    // Implementation can be added here if required.
}

/**
 * Adds a listener for changes to a specific Articy object.
 * Registers a lambda function to handle property changes.
 * @param Object The Articy object to listen to.
 * @param ChangedFunction The function to call when a property changes.
 */
void UArticyObjectNotificationManager::AddListener(UArticyBaseObject* Object, FArticyPropertyChangedFunction ChangedFunction)
{
    Object->ReportChanged.AddLambda([ChangedFunction](FArticyChangedProperty ChangedProperty)
        {
            ChangedFunction(ChangedProperty);
        });
}

/**
 * Removes listeners for Articy objects matching the given filter.
 * Resolves the object based on the filter and removes its listeners.
 * @param Filter The filter string for identifying Articy objects.
 */
void UArticyObjectNotificationManager::RemoveListeners(const FString& Filter)
{
    FString ObjectName, ObjectInstance;
    SplitInstance(Filter, ObjectName, ObjectInstance);
    UArticyObject* Object;
    auto DB = UArticyDatabase::Get(this);
    if (Filter.StartsWith(TEXT("0x")))
    {
        Object = DB->GetObject<UArticyObject>(FArticyId{ ArticyHelpers::HexToUint64(ObjectName) }, FCString::Atod(*ObjectInstance));
    }
    else if (Filter.IsNumeric())
    {
        Object = DB->GetObject<UArticyObject>(FArticyId{ FCString::Strtoui64(*ObjectName, nullptr, 10) }, FCString::Atod(*ObjectInstance));
    }
    else
    {
        Object = DB->GetObjectByName(*ObjectName, FCString::Atod(*ObjectInstance));
    }

    if (Object)
    {
        RemoveListeners(Object);
    }
}

/**
 * Removes listeners from a specific Articy object.
 * Clears all change notifications for the given object.
 * @param Object The Articy object to remove listeners from.
 */
void UArticyObjectNotificationManager::RemoveListeners(UArticyBaseObject* Object)
{
    Object->ReportChanged.Clear();
}
