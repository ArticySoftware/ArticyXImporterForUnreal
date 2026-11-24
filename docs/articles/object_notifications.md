\page objectNotifications Object Notifications

Objects imported from **articy:draft** change while the game runs: a script raises a character's
morale, an item is marked as used, a location gets a new display name. The
[**ArticyObjectNotificationManager**](@ref UArticyObjectNotificationManager) tells you about these
changes, so a health bar or a quest log can update itself instead of polling the database.

As in the Unity importer, you can listen to a **single object**, to **all objects of a type** or to
**all objects**, and narrow each of these down to specific properties with a **filter** and with
**flags**. All listeners are kept by the manager rather than by the objects, which means they can
also be removed in one place.

Every runtime database has a notification manager of its own. Get it from the database with
[`GetObjectNotifications`](@ref UArticyDatabase::GetObjectNotifications), or directly with
[`UArticyObjectNotificationManager::Get`](@ref UArticyObjectNotificationManager::Get).

---

### Listening to an Object

@tabs{
@tab{ C++ | cpp |
```cpp
UArticyDatabase* DB = UArticyDatabase::Get(WorldContext);
UArticyObjectNotificationManager* Notifications = DB->GetObjectNotifications();

UArticyObject* Manfred = DB->GetObjectByName(TEXT("Chr_Manfred"));

// Any property of this object
Notifications->AddListener(Manfred, [](const FArticyChangedProperty& Changed)
{
	UE_LOG(LogTemp, Log, TEXT("%s changed on %s"), *Changed.Property.ToString(), *Changed.ObjectReference->GetName());
});

// Only the morale value inside its Morale feature
FArticyListenerHandle Handle = Notifications->AddListener(Manfred, TEXT("Morale.MoraleValue"),
	[this](const FArticyChangedProperty& Changed) { RefreshMoraleBar(); });
```

The callback receives an [`FArticyChangedProperty`](@ref FArticyChangedProperty) with the object
and the name of the property that changed. Keep the returned
[`FArticyListenerHandle`](@ref FArticyListenerHandle) if you want to remove the listener later.
}
@tab{ Blueprint | uebp |
- **Get Articy Object Notifications** returns the manager; the database also has a
  **Get Object Notifications** node.
- **Add Object Listener** takes the object, the filter and an event with a *Changed Property*
  input, and returns the handle of the listener.
}
}

An object and each of its [clones](@ref cloningObjects) are separate instances: a listener added
for a clone is not told about changes of the original, and the other way round.

---

### Listening to a Type

A type-based listener is told about changes on **every object of a class**, including the classes
derived from it. This covers objects that are loaded or cloned after the listener was added.

@tabs{
@tab{ C++ | cpp |
```cpp
// Every character, whichever property
Notifications->AddListener<UManiacManfredCharacter>(TEXT("*"), [](const FArticyChangedProperty& Changed) { /* ... */ });

// The same with a class object, e.g. one picked in the editor
Notifications->AddListener(UManiacManfredCharacter::StaticClass(), TEXT("DisplayName"), OnDisplayNameChanged);

// Every object with a display name, by its interface
Notifications->AddListener(UArticyObjectWithDisplayName::StaticClass(), TEXT("DisplayName"), OnDisplayNameChanged);

// By articy type, as returned by an object or the type system
Notifications->AddListener(Manfred->GetArticyType(), TEXT("Morale.*"), OnMoraleChanged);
```
}
@tab{ Blueprint | uebp |
- **Add Type Listener** takes the class instead of an object.
- **Add Listener** listens to all objects, whatever their type.
}
}

To listen to all objects regardless of their type, leave out the object and the type:

```cpp
// Whoever has an Inventory feature: tell me when the gold count changes
Notifications->AddListener(TEXT("Inventory.GoldCount"), OnGoldChanged);
```

---

### The Filter

The filter selects the **properties** a listener is told about. It is matched against the name of
the changed property, ignoring case:

| Filter | Matches |
|---|---|
| `DisplayName` | The property `DisplayName` of the object. |
| `Morale.MoraleValue` | The property `MoraleValue` inside the feature `Morale`. |
| `Morale.*` | Every property inside the feature `Morale`. |
| `*.MoraleValue` | The property `MoraleValue` inside any feature. |
| `*` or an empty filter | Every property. |

Template properties are always named `Feature.Property`, using the technical names from
*articy:draft*. That is also the name passed to the callback in
[`FArticyChangedProperty::Property`](@ref FArticyChangedProperty::Property), while
`ObjectReference` is the object owning the feature. Besides `*`, the wildcard `?` stands for a
single character.

> [!note]
> In earlier versions of the plugin the filter named the **object** to listen to, by its ID or
> technical name. It now names the property, as it does in the Unity importer. Pass the object
> itself to listen to a specific one.

---

### The Flags

[`EArticyTypeProperties`](@ref EArticyTypeProperties) flags limit a listener to properties of a
certain origin. They are combined with the filter, and all flags are set if you do not pass any.

| Flag | Properties |
|---|---|
| `Template` | Properties inside a feature. |
| `General` | Properties of the object's own type, like `DisplayName` or `Text`. |
| `ArticyObject` | Properties every articy object has, like `Id` or `TechnicalName`. |
| `IncludeBaseType` | In addition to `General`: the properties an object inherits from its base type. For an object with a template these are the properties of the type the template is made for, e.g. the `DisplayName` of an Entity. |

```cpp
// All template properties of all characters, but none of their basic properties
Notifications->AddListener<UManiacManfredCharacter>(TEXT("*"), EArticyTypeProperties::Template, OnTemplateChanged);

// The basic properties of an object with a template, which it inherits from its base type
Notifications->AddListener(Manfred, TEXT("*"), EArticyTypeProperties::General | EArticyTypeProperties::IncludeBaseType, OnBasicsChanged);
```

In Blueprint the flags are the *Flags* bitmask of the **Add ... Listener** nodes.

---

### What Counts as a Change

Listeners are notified when a property is set

- by a script from *articy:draft*, e.g. `setProp`, `incrementProp` or `decrementProp`,
- through [`SetProp`](@ref IArticyReflectable::SetProp) in C++, or
- when you report it with [`ReportChanged`](@ref UArticyObjectNotificationManager::ReportChanged).

Assigning a property directly, in C++ or with a Blueprint **Set** node, is a plain write to memory
that the plugin cannot observe. Report such a change yourself if listeners should know about it:

@tabs{
@tab{ C++ | cpp |
```cpp
Manfred->DisplayName = FText::FromString(TEXT("Advanced Manfred 2.0"));
Notifications->ReportChanged(Manfred, TEXT("DisplayName"));
```
}
@tab{ Blueprint | uebp |
- **Report Changed** on the manager takes the object and the name of the property.
}
}

Nothing is reported while a flow player is [exploring branches](@ref dialoguesFlowTraversal): the
instructions it runs to look ahead work on copies of the objects, and their changes are discarded.

---

### Removing Listeners

@tabs{
@tab{ C++ | cpp |
```cpp
// A single listener
Notifications->RemoveListener(Handle);

// All listeners added for one object, or for one type
Notifications->RemoveListeners(Manfred);
Notifications->RemoveListeners<UManiacManfredCharacter>();

// Everything
Notifications->RemoveAllListeners();
```
}
@tab{ Blueprint | uebp |
- **Remove Listener** takes the handle returned when the listener was added.
- **Remove Object Listeners** / **Remove Type Listeners** remove what was added for an object or a class.
- **Remove All Listeners** removes every listener of the manager.
}
}

> [!note]
> A C++ listener is a function the manager keeps calling until it is removed. If it captures an
> object, such as a widget or an actor, remove the listener before that object goes away. Listeners
> for an object that was unloaded are dropped automatically, and so is the whole manager when its
> database is unloaded.
