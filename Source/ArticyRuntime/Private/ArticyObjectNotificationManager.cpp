//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyObjectNotificationManager.h"
#include "ArticyBaseObject.h"
#include "ArticyDatabase.h"
#include "ArticyRuntimeModule.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace
{
    /** Returns the owner of a feature and qualifies Property with the feature's name. */
    UArticyBaseObject* ResolveFeatureOwner(UArticyBaseObject* Object, FString& Property)
    {
        if (!Object->IsA<UArticyBaseFeature>())
            return Object;

        UArticyBaseObject* Owner = Cast<UArticyBaseObject>(Object->GetOuter());
        if (!Owner)
            return Object;

        // The feature is named after the owner's property holding it
        for (TFieldIterator<FObjectProperty> It(Owner->GetClass()); It; ++It)
        {
            if (It->GetObjectPropertyValue_InContainer(Owner) == Object)
            {
                Property = It->GetNameCPP() + TEXT(".") + Property;
                return Owner;
            }
        }

        return Object;
    }

    /** Returns the flag describing a property, and whether it is inherited from a base type. */
    EArticyTypeProperties GetPropertyKind(const UArticyBaseObject* Object, const FString& Property, bool& bOutInherited)
    {
        bOutInherited = false;

        if (Property.Contains(TEXT(".")))
            return EArticyTypeProperties::Template;

        const FProperty* Reflected = Object->GetProperty(*Property);
        const UClass* DeclaringClass = Reflected ? Reflected->GetOwnerClass() : nullptr;
        if (!DeclaringClass)
            return EArticyTypeProperties::General;

        // Declared by the plugin's own classes
        if (DeclaringClass->GetOutermost() == UArticyBaseObject::StaticClass()->GetOutermost())
            return EArticyTypeProperties::ArticyObject;

        bOutInherited = DeclaringClass != Object->GetClass();
        return EArticyTypeProperties::General;
    }

    /** Returns the class generated for an articy type, or nullptr if there is none. */
    const UClass* FindClassOfType(const FArticyType& Type)
    {
        if (Type.CPPType.IsEmpty())
            return nullptr;

        TArray<UClass*> Classes;
        GetDerivedClasses(UArticyBaseObject::StaticClass(), Classes, true);

        for (const UClass* Class : Classes)
        {
            // UClass names drop the U prefix the generated CPPType carries
            if (Class->IsChildOf(UArticyBaseFeature::StaticClass()))
                continue;
            if (Class->GetName() == Type.CPPType || (Class->GetPrefixCPP() + Class->GetName()) == Type.CPPType)
                return Class;
        }

        return nullptr;
    }

    /** Converts a Blueprint bitmask into flags. */
    EArticyTypeProperties ToTypeProperties(int32 Flags)
    {
        return static_cast<EArticyTypeProperties>(Flags & static_cast<int32>(EArticyTypeProperties::All));
    }
}

//---------------------------------------------------------------------------//

bool UArticyObjectNotificationManager::FListener::IsStale() const
{
    if (Scope == EListenerScope::Object && !Object.IsValid())
        return true;
    if (Scope == EListenerScope::Type && !Type.IsValid())
        return true;

    return !Function && !Delegate.IsBound();
}

bool UArticyObjectNotificationManager::FListener::Matches(const UArticyBaseObject* ChangedObject, const FString& Property,
    EArticyTypeProperties Kind, bool bInherited) const
{
    if (Scope == EListenerScope::Object && Object.Get() != ChangedObject)
        return false;

    if (Scope == EListenerScope::Type)
    {
        const UClass* Class = Type.Get();
        if (!Class)
            return false;

        const bool bIsOfType = Class->HasAnyClassFlags(CLASS_Interface)
            ? ChangedObject->GetClass()->ImplementsInterface(Class)
            : ChangedObject->IsA(Class);
        if (!bIsOfType)
            return false;
    }

    if (!EnumHasAnyFlags(Flags, Kind))
        return false;
    if (bInherited && !EnumHasAnyFlags(Flags, EArticyTypeProperties::IncludeBaseType))
        return false;

    return Filter.IsEmpty() || Property.MatchesWildcard(Filter, ESearchCase::IgnoreCase);
}

//---------------------------------------------------------------------------//

/**
 * Gets the notification manager of the database of a world.
 * @param WorldContext Any UObject in the target world.
 * @return The notification manager, or nullptr if there is no database.
 */
UArticyObjectNotificationManager* UArticyObjectNotificationManager::Get(const UObject* WorldContext)
{
    const UArticyDatabase* Database = WorldContext ? UArticyDatabase::Get(WorldContext) : nullptr;
    return Database ? Database->GetObjectNotifications() : nullptr;
}

/**
 * Registers a listener.
 * @param Listener The listener to register.
 * @return The handle of the listener.
 */
FArticyListenerHandle UArticyObjectNotificationManager::AddListenerInternal(FListener&& Listener)
{
    FArticyListenerHandle Handle;
    if (Listener.IsStale())
    {
        UE_LOG(LogArticyRuntime, Warning, TEXT("Cannot add a listener without a callback or without the object or type to listen to."));
        return Handle;
    }

    Listener.Id = ++LastListenerId;
    Handle.Id = Listener.Id;
    Listeners.Add(MoveTemp(Listener));

    return Handle;
}

/**
 * Adds a listener for property changes on any object.
 * @param Filter The property filter, which can contain wildcards.
 * @param ChangedFunction The function to call when a property changes.
 */
FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const FString& Filter, FArticyPropertyChangedFunction ChangedFunction)
{
    return AddListener(Filter, EArticyTypeProperties::All, MoveTemp(ChangedFunction));
}

/**
 * Adds a listener for property changes on any object.
 * @param Filter The property filter, which can contain wildcards.
 * @param Flags The types of properties to include in notifications.
 * @param ChangedFunction The function to call when a property changes.
 */
FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const FString& Filter, EArticyTypeProperties Flags,
    FArticyPropertyChangedFunction ChangedFunction)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Any;
    Listener.Filter = Filter;
    Listener.Flags = Flags;
    Listener.Function = MoveTemp(ChangedFunction);

    return AddListenerInternal(MoveTemp(Listener));
}

/**
 * Adds a listener for changes to a specific Articy object.
 * @param Object The Articy object to listen to.
 * @param ChangedFunction The function to call when a property changes.
 */
FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const UArticyBaseObject* Object, FArticyPropertyChangedFunction ChangedFunction)
{
    return AddListener(Object, FString(), EArticyTypeProperties::All, MoveTemp(ChangedFunction));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const UArticyBaseObject* Object, const FString& Filter,
    FArticyPropertyChangedFunction ChangedFunction)
{
    return AddListener(Object, Filter, EArticyTypeProperties::All, MoveTemp(ChangedFunction));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const UArticyBaseObject* Object, const FString& Filter,
    EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Object;
    Listener.Object = Object;
    Listener.Filter = Filter;
    Listener.Flags = Flags;
    Listener.Function = MoveTemp(ChangedFunction);

    return AddListenerInternal(MoveTemp(Listener));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const UClass* Type, const FString& Filter,
    FArticyPropertyChangedFunction ChangedFunction)
{
    return AddListener(Type, Filter, EArticyTypeProperties::All, MoveTemp(ChangedFunction));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const UClass* Type, const FString& Filter,
    EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Type;
    Listener.Type = Type;
    Listener.Filter = Filter;
    Listener.Flags = Flags;
    Listener.Function = MoveTemp(ChangedFunction);

    return AddListenerInternal(MoveTemp(Listener));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const FArticyType& Type, const FString& Filter,
    FArticyPropertyChangedFunction ChangedFunction)
{
    return AddListener(Type, Filter, EArticyTypeProperties::All, MoveTemp(ChangedFunction));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddListener(const FArticyType& Type, const FString& Filter,
    EArticyTypeProperties Flags, FArticyPropertyChangedFunction ChangedFunction)
{
    const UClass* Class = FindClassOfType(Type);
    if (!Class)
    {
        UE_LOG(LogArticyRuntime, Warning, TEXT("Cannot add a listener for articy type %s, no class is generated for it."), *Type.TechnicalName);
        return FArticyListenerHandle();
    }

    return AddListener(Class, Filter, Flags, MoveTemp(ChangedFunction));
}

//---------------------------------------------------------------------------//

FArticyListenerHandle UArticyObjectNotificationManager::AddPropertyListener(const FString& Filter,
    const FArticyPropertyChangedDelegate& OnChanged, int32 Flags)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Any;
    Listener.Filter = Filter;
    Listener.Flags = ToTypeProperties(Flags);
    Listener.Delegate = OnChanged;

    return AddListenerInternal(MoveTemp(Listener));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddObjectListener(const UArticyBaseObject* Object, const FString& Filter,
    const FArticyPropertyChangedDelegate& OnChanged, int32 Flags)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Object;
    Listener.Object = Object;
    Listener.Filter = Filter;
    Listener.Flags = ToTypeProperties(Flags);
    Listener.Delegate = OnChanged;

    return AddListenerInternal(MoveTemp(Listener));
}

FArticyListenerHandle UArticyObjectNotificationManager::AddTypeListener(TSubclassOf<UArticyBaseObject> Type, const FString& Filter,
    const FArticyPropertyChangedDelegate& OnChanged, int32 Flags)
{
    FListener Listener;
    Listener.Scope = EListenerScope::Type;
    Listener.Type = Type.Get();
    Listener.Filter = Filter;
    Listener.Flags = ToTypeProperties(Flags);
    Listener.Delegate = OnChanged;

    return AddListenerInternal(MoveTemp(Listener));
}

//---------------------------------------------------------------------------//

/**
 * Removes a listener.
 * @param Handle The handle of the listener.
 * @return True if a listener was removed.
 */
bool UArticyObjectNotificationManager::RemoveListener(FArticyListenerHandle Handle)
{
    if (!Handle.IsValid())
        return false;

    return Listeners.RemoveAll([&Handle](const FListener& Listener) { return Listener.Id == Handle.Id; }) > 0;
}

/**
 * Removes listeners from a specific Articy object.
 * Clears all change notifications for the given object.
 * @param Object The Articy object to remove listeners from.
 */
int32 UArticyObjectNotificationManager::RemoveListeners(const UArticyBaseObject* Object)
{
    return Listeners.RemoveAll([Object](const FListener& Listener)
        {
            return Listener.Scope == EListenerScope::Object && Listener.Object.Get() == Object;
        });
}

/**
 * Removes the listeners added for a type.
 * @param Type The class the listeners were added for.
 * @return The number of listeners removed.
 */
int32 UArticyObjectNotificationManager::RemoveListeners(const UClass* Type)
{
    return Listeners.RemoveAll([Type](const FListener& Listener)
        {
            return Listener.Scope == EListenerScope::Type && Listener.Type.Get() == Type;
        });
}

/**
 * Removes all listeners.
 */
void UArticyObjectNotificationManager::RemoveAllListeners()
{
    Listeners.Empty();
}

//---------------------------------------------------------------------------//

/**
 * Reports a changed property to the listeners.
 * @param Object The object owning the property, or the feature it is in.
 * @param Property The name of the property.
 */
void UArticyObjectNotificationManager::ReportChanged(UArticyBaseObject* Object, const FString& Property)
{
    if (!Object || Listeners.Num() == 0)
        return;

    Listeners.RemoveAll([](const FListener& Listener) { return Listener.IsStale(); });

    FString QualifiedProperty = Property;
    UArticyBaseObject* Owner = ResolveFeatureOwner(Object, QualifiedProperty);

    bool bInherited = false;
    const EArticyTypeProperties Kind = GetPropertyKind(Owner, QualifiedProperty, bInherited);

    // Collected first, as listeners may add or remove listeners
    TArray<int64> Matching;
    for (const FListener& Listener : Listeners)
    {
        if (Listener.Matches(Owner, QualifiedProperty, Kind, bInherited))
            Matching.Add(Listener.Id);
    }

    FArticyChangedProperty ChangedProperty;
    ChangedProperty.ObjectReference = Owner;
    ChangedProperty.Property = *QualifiedProperty;

    for (const int64 Id : Matching)
    {
        const FListener* Listener = Listeners.FindByPredicate([Id](const FListener& Item) { return Item.Id == Id; });
        if (!Listener)
            continue;

        // Called on a copy, as the listener may remove itself
        if (Listener->Function)
        {
            const FArticyPropertyChangedFunction Function = Listener->Function;
            Function(ChangedProperty);
        }
        else
        {
            const FArticyPropertyChangedDelegate Delegate = Listener->Delegate;
            Delegate.ExecuteIfBound(ChangedProperty);
        }
    }
}

/**
 * Reports a changed property to the manager of the object's database.
 * @param Object The object owning the property, or the feature it is in.
 * @param Property The name of the property.
 */
void UArticyObjectNotificationManager::ReportObjectChanged(UArticyBaseObject* Object, FName Property)
{
    if (!Object)
        return;

    // Runtime objects are outered to their database; changes in shadow state are discarded
    const UArticyDatabase* Database = Object->GetTypedOuter<UArticyDatabase>();
    if (!Database || Database->IsInShadowState())
        return;

    UArticyObjectNotificationManager* Manager = Database->FindObjectNotifications();
    if (Manager && Manager->Listeners.Num() > 0)
        Manager->ReportChanged(Object, Property.ToString());
}
