\page typeSystem Type System

The **ArticyImporter** plugin for Unreal Engine provides a built-in **Type System**, which offers an optimized way to get meta-information about types, classes, and properties without using Unreal's built-in reflection system. The Articy Type System operates similarly to Unreal's reflection but is specifically tailored for working with Articy data types and is optimized for performance.

### Overview

The Type System automatically generates and imports classes and types from Articy content, and allows you to access information about those types at runtime. It contains functionality to retrieve property data, type hierarchy information, template structures, and more.

---

### Accessing Type Information

You can access type information in Unreal Engine through the `UArticyTypeSystem` class, which stores metadata about each Articy class and its associated properties.

#### Example: Retrieving Type Information for an Entity
You can retrieve the type for any object by its name:
```cpp
FArticyType EntityType = UArticyTypeSystem::Get()->GetArticyType("Entity");

// Iterate over properties of the Entity class
for (const FArticyPropertyInfo& Prop : EntityType.GetProperties())
{
    UE_LOG(LogTemp, Warning, TEXT("Property: %s"), *Prop.TechnicalName);
}
```

In this example, we retrieve the **Entity** type, and then iterate over its properties, printing the technical names of each property.

---

### FArticyType

At the core of the type system is the `FArticyType` struct, which represents a single Articy class, enum, or template. It stores information about a specific type and includes methods to retrieve detailed property information.

#### Key Properties of `FArticyType`:
- **TechnicalName:** The technical name of the type.
- **CPPType:** The name of the generated C++ class for the type.
- **Properties:** A list of all properties of this type.
- **Features:** A list of features available on this type (for templates).
- **IsEnum:** Whether the type is an enum or not.
- **HasTemplate:** Whether the type has an associated template.

#### Key Methods of `FArticyType`:
- `GetProperties()`: Returns a list of properties defined on this type.
- `GetPropertiesInFeature()`: Returns properties in a specific feature (for templates).
- `GetEnumValue()`: Returns metadata for an enum value.
- `GetProperty()`: Returns the `FArticyPropertyInfo` for a specific property by name.

#### Example: Retrieving Properties of a Type
```cpp
FArticyType CharacterType = UArticyTypeSystem::Get()->GetArticyType("Character");

for (const FArticyPropertyInfo& Prop : CharacterType.GetProperties())
{
    UE_LOG(LogTemp, Warning, TEXT("Property: %s of Type: %s"), *Prop.TechnicalName, *Prop.PropertyType);
}
```
In this example, we retrieve the `Character` type and iterate over its properties, logging the technical name and the property type.

---

### FArticyPropertyInfo

`FArticyPropertyInfo` is a struct that holds information about an individual property within a type. This includes the technical name, property type, localization key, and whether the property belongs to a template.

#### Key Properties of `FArticyPropertyInfo`:
- **TechnicalName:** The technical name of the property (used to reference it in code).
- **PropertyType:** The data type of the property (e.g., `string`, `int`).
- **LocaKey_DisplayName:** The localization key for the display name.
- **IsTemplateProperty:** Whether the property belongs to a template feature.

#### Example: Retrieving and Modifying Property Values
You can use `FArticyPropertyInfo` to dynamically access or modify property values on Articy objects.

```cpp
FArticyType CharacterType = UArticyTypeSystem::Get()->GetArticyType("Character");
FArticyPropertyInfo BackstoryProperty = CharacterType.GetProperty("Basic.Backstory");

// Get the property type (e.g., string)
UE_LOG(LogTemp, Warning, TEXT("Backstory Property Type: %s"), *BackstoryProperty.PropertyType);
```

Here, we retrieve the `Backstory` property from the `Character` type and print its type.

---

### Working with Templates

The type system makes it easy to access properties within templates. Templates in **articy:draft** are treated as special classes in Unreal, with their own sets of properties (features). The `GetPropertiesInFeature()` method allows you to access all properties under a specific feature within a template.

#### Example: Accessing Template Properties
```cpp
FArticyType CharacterType = UArticyTypeSystem::Get()->GetArticyType("Character");

// Get properties in the "Attributes" feature
TArray<FArticyPropertyInfo> Attributes = CharacterType.GetPropertiesInFeature("Attributes");

for (const FArticyPropertyInfo& Attr : Attributes)
{
    UE_LOG(LogTemp, Warning, TEXT("Attribute: %s"), *Attr.TechnicalName);
}
```
In this example, we access all properties in the "Attributes" feature of the `Character` type.

---

### Enums in the Type System

Enums in **articy:draft** are handled by the type system as well, with support for retrieving enum values and their metadata. Each enum is treated as a separate type, and its values are represented by `FArticyEnumValueInfo`.

#### Example: Retrieving Enum Values
```cpp
FArticyType WeaponType = UArticyTypeSystem::Get()->GetArticyType("WeaponType");

for (const FArticyEnumValueInfo& EnumValue : WeaponType.EnumValues)
{
    UE_LOG(LogTemp, Warning, TEXT("Enum: %s = %d"), *EnumValue.DisplayName, EnumValue.Value);
}
```
In this example, we retrieve the enum values for the `WeaponType` enum and log their display names and numeric values.

---

The **ArticyImporter** plugin's Type System in Unreal Engine provides a flexible and performant way to access metadata about Articy objects, templates, and enums. By leveraging `FArticyType`, `FArticyPropertyInfo`, and other components, you can dynamically access and manipulate Articy-generated content, reducing the need for hardcoded logic and improving your workflow when building complex narrative-driven projects.

This system is particularly useful for creating dynamic UI elements (e.g., character sheets), implementing data-driven gameplay mechanics, or building custom save systems based on Articy data.
