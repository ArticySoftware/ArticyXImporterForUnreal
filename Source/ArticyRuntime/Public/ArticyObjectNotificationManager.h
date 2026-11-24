//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Templates/SubclassOf.h"
#include "UObject/WeakObjectPtr.h"
#include "ArticyBaseObject.h"
#include "ArticyChangedProperty.h"
#include "ArticyType.h"
#include "ArticyObjectNotificationManager.generated.h"

/**
 * Function type for handling changes in Articy properties.
 * @param ChangedProperty The property that has changed.
 */
using FArticyPropertyChangedFunction = TFunction<void(const FArticyChangedProperty&)>;

/** @brief Blueprint callback for a changed property of an articy object. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FArticyPropertyChangedDelegate, const FArticyChangedProperty&, ChangedProperty);

/**
 * Enum defining the types of Articy properties to include in notifications.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EArticyTypeProperties : uint8 {
    None = 0 UMETA(Hidden),
    /** Includes base type properties. */
    IncludeBaseType = 1,
    /** Includes template properties. */
    Template = 2,
    /** Includes general properties. */
    General = 4,
    /** Includes Articy object-specific properties. */
    ArticyObject = 8,
    /** Includes all types of properties. */
    All = 15 UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EArticyTypeProperties);

/**
 * @brief Identifies a listener of UArticyObjectNotificationManager, to remove it again.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyListenerHandle
{
    GENERATED_BODY()

public:
    /** @brief Returns true if the handle refers to a listener. */
    bool IsValid() const { return Id != 0; }

    /** @brief Makes the handle refer to no listener. */
    void Reset() { Id = 0; }

    bool operator==(const FArticyListenerHandle& Other) const { return Id == Other.Id; }
    bool operator!=(const FArticyListenerHandle& Other) const { return Id != Other.Id; }

private:
    friend class UArticyObjectNotificationManager;

    UPROPERTY()
    int64 Id = 0;
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
     * Gets the notification manager of the database of a world.
     * @param WorldContext Any UObject in the target world.
     * @return Pointer to the notification manager, or nullptr if there is no database.
     */
    UFUNCTION(BlueprintPure, Category = "Articy|Notifications", meta = (WorldContext = "WorldContext", DisplayName = "Get Articy Object Notifications"))
    static UArticyObjectNotificationManager* Get(const UObject* WorldContext);

    //---------------------------------------------------------------------------//

    /**
     * @brief Adds a listener for property changes on any object.
     * @param Filter The property filter, which can contain wildcards.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const FString& Filter, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on any object.
     * @param Filter The property filter, which can contain wildcards.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * Adds a listener for changes to a specific Articy object.
     * @param Object The Articy object to listen to.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const UArticyBaseObject* Object, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on a single object.
     * @param Object The Articy object to listen to.
     * @param Filter The property filter, which can contain wildcards.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const UArticyBaseObject* Object, const FString& Filter, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on a single object.
     * @param Object The Articy object to listen to.
     * @param Filter The property filter, which can contain wildcards.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const UArticyBaseObject* Object, const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on all objects of a type.
     * @param Type The class or interface to listen to, including derived classes.
     * @param Filter The property filter, which can contain wildcards.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const UClass* Type, const FString& Filter, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on all objects of a type.
     * @param Type The class or interface to listen to, including derived classes.
     * @param Filter The property filter, which can contain wildcards.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    FArticyListenerHandle AddListener(const UClass* Type, const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on all objects of an articy type.
     * @param Type The articy type to listen to.
     * @param Filter The property filter, which can contain wildcards.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener, invalid if the type has no generated class.
     */
    FArticyListenerHandle AddListener(const FArticyType& Type, const FString& Filter, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on all objects of an articy type.
     * @param Type The articy type to listen to.
     * @param Filter The property filter, which can contain wildcards.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener, invalid if the type has no generated class.
     */
    FArticyListenerHandle AddListener(const FArticyType& Type, const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction);

    /**
     * @brief Adds a listener for property changes on all objects of type TType.
     * @param Filter The property filter, which can contain wildcards.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    template<typename TType>
    FArticyListenerHandle AddListener(const FString& Filter, FArticyPropertyChangedFunction ChangedFunction)
    {
        return AddListener(TType::StaticClass(), Filter, MoveTemp(ChangedFunction));
    }

    /**
     * @brief Adds a listener for property changes on all objects of type TType.
     * @param Filter The property filter, which can contain wildcards.
     * @param Flags The types of properties to include in notifications.
     * @param ChangedFunction The function to call when a property changes.
     * @return The handle of the listener.
     */
    template<typename TType>
    FArticyListenerHandle AddListener(const FString& Filter, EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction)
    {
        return AddListener(TType::StaticClass(), Filter, Flags, MoveTemp(ChangedFunction));
    }

    //---------------------------------------------------------------------------//

    /**
     * @brief Adds a listener for property changes on any object.
     * @param Filter The property filter, which can contain wildcards.
     * @param OnChanged The event to call when a property changes.
     * @param Flags The types of properties to include in notifications.
     * @return The handle of the listener.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications", meta = (DisplayName = "Add Listener"))
    FArticyListenerHandle AddPropertyListener(const FString& Filter, const FArticyPropertyChangedDelegate& OnChanged,
        UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/ArticyRuntime.EArticyTypeProperties")) int32 Flags = 15);

    /**
     * @brief Adds a listener for property changes on a single object.
     * @param Object The Articy object to listen to.
     * @param Filter The property filter, which can contain wildcards.
     * @param OnChanged The event to call when a property changes.
     * @param Flags The types of properties to include in notifications.
     * @return The handle of the listener.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    FArticyListenerHandle AddObjectListener(const UArticyBaseObject* Object, const FString& Filter, const FArticyPropertyChangedDelegate& OnChanged,
        UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/ArticyRuntime.EArticyTypeProperties")) int32 Flags = 15);

    /**
     * @brief Adds a listener for property changes on all objects of a type.
     * @param Type The class to listen to, including derived classes.
     * @param Filter The property filter, which can contain wildcards.
     * @param OnChanged The event to call when a property changes.
     * @param Flags The types of properties to include in notifications.
     * @return The handle of the listener.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    FArticyListenerHandle AddTypeListener(TSubclassOf<UArticyBaseObject> Type, const FString& Filter, const FArticyPropertyChangedDelegate& OnChanged,
        UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/ArticyRuntime.EArticyTypeProperties")) int32 Flags = 15);

    //---------------------------------------------------------------------------//

    /**
     * @brief Removes a listener.
     * @param Handle The handle of the listener.
     * @return True if a listener was removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    bool RemoveListener(FArticyListenerHandle Handle);

    /**
     * Removes listeners from a specific Articy object.
     * @param Object The Articy object to remove listeners from.
     * @return The number of listeners removed.
     */
    int32 RemoveListeners(const UArticyBaseObject* Object);

    /**
     * @brief Removes the listeners added for a type.
     * @param Type The class the listeners were added for.
     * @return The number of listeners removed.
     */
    int32 RemoveListeners(const UClass* Type);

    /**
     * @brief Removes the listeners added for type TType.
     * @return The number of listeners removed.
     */
    template<typename TType>
    int32 RemoveListeners()
    {
        return RemoveListeners(TType::StaticClass());
    }

    /**
     * @brief Removes the listeners added for a single object.
     * @param Object The object instance the listeners were added for.
     * @return The number of listeners removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    int32 RemoveObjectListeners(const UArticyBaseObject* Object) { return RemoveListeners(Object); }

    /**
     * @brief Removes the listeners added for a type.
     * @param Type The class the listeners were added for.
     * @return The number of listeners removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    int32 RemoveTypeListeners(TSubclassOf<UArticyBaseObject> Type) { return RemoveListeners(Type.Get()); }

    /** @brief Removes all listeners. */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    void RemoveAllListeners();

    /** @brief Gets the number of listeners. */
    UFUNCTION(BlueprintPure, Category = "Articy|Notifications")
    int32 GetNumListeners() const { return Listeners.Num(); }

    //---------------------------------------------------------------------------//

    /**
     * @brief Reports a changed property to the listeners.
     * @param Object The object owning the property, or the feature it is in.
     * @param Property The name of the property.
     */
    UFUNCTION(BlueprintCallable, Category = "Articy|Notifications")
    void ReportChanged(UArticyBaseObject* Object, const FString& Property);

    /**
     * @brief Reports a changed property to the manager of the object's database.
     * @param Object The object owning the property, or the feature it is in.
     * @param Property The name of the property.
     */
    static void ReportObjectChanged(UArticyBaseObject* Object, FName Property);

private:
    /** What a listener is added for. */
    enum class EListenerScope : uint8
    {
        Any,
        Object,
        Type
    };

    /** A registered listener. */
    struct FListener
    {
        int64 Id = 0;
        EListenerScope Scope = EListenerScope::Any;
        TWeakObjectPtr<const UArticyBaseObject> Object;
        TWeakObjectPtr<const UClass> Type;
        FString Filter;
        EArticyTypeProperties Flags = EArticyTypeProperties::All;
        /** The callback of a C++ listener. */
        FArticyPropertyChangedFunction Function;
        /** The callback of a Blueprint listener. */
        FArticyPropertyChangedDelegate Delegate;

        /** Returns true if the listener can never be called again. */
        bool IsStale() const;
        /** Returns true if the listener is to be notified about this change. */
        bool Matches(const UArticyBaseObject* ChangedObject, const FString& Property, EArticyTypeProperties Kind, bool bInherited) const;
    };

    /** Registers a listener and returns its handle. */
    FArticyListenerHandle AddListenerInternal(FListener&& Listener);

    /** All listeners, in the order they were added. */
    TArray<FListener> Listeners;

    /** The id of the most recently added listener. */
    int64 LastListenerId = 0;
};
