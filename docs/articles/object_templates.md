\page objectTemplates Object Templates

When exporting data from **articy:draft X** into Unreal, the **ArticyXImporter** plugin imports not only the objects but also their templates. This section covers how templates work in Unreal and how to interact with them effectively using C++ and Blueprint.

### Primer to Object Templates in articy:draft

Every object in articy:draft (like Entity, FlowFragment, Location, etc.) has predefined properties such as **DisplayName**, **TechnicalName**, **ID**, **Text**, and **Attachments**. However, there are cases where custom properties are required for specific gameplay mechanics. For example, you might want every NPC to have a "gold value" property to track how much gold is awarded when they are defeated.

This is where **Object Templates** and **Features** come in. Templates allow you to extend the base properties of articy objects by attaching custom features and properties. Templates are always bound to a specific object type, and each template can be composed of one or more features that define custom properties.

- **Template**: A container bound to a specific object type (e.g., Entity or FlowFragment).
- **Feature**: A reusable set of properties that can be attached to multiple templates.
- **Property**: A specific piece of information stored in a feature, such as a number, string, or boolean value.

For example, you could create a **LootInfo** feature with a **Gold** property and attach it to an **NPC** entity template. Later, you could reuse the **LootInfo** feature for other object templates like **Chest**.

### Accessing Template Data in Unreal

When articy templates are exported to Unreal, they are translated into classes with properties corresponding to the features and custom properties defined in articy:draft. One key difference in Unreal is that you do not need to access a separate `Template` property when interacting with template data. You can directly access the feature properties using the object's class.

**Example: Accessing template properties directly**:

```cpp
UManiacManfredCharacter* Manfred = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredCharacter>("Chr_Manfred");

// Accessing template feature properties directly
float ManfredAge = Manfred->Character.Age;
FString ManfredMotivation = Manfred->Character.Motivation;
```

As seen here, there's no need to prefix the property with `Template`—just access the feature (`Character`) and its properties (`Age`, `Motivation`) directly.

---

### Working with Specific Property Types

Unreal provides direct access to the various types of properties defined in articy templates, such as **Booleans**, **Numbers**, **Text**, **Slots**, **Strips**, and more.

#### Boolean, Number, Text

These basic types map easily to their Unreal counterparts and can be accessed and modified directly.

```cpp
UManiacManfredCharacter* Manfred = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredCharacter>("Chr_Manfred");

float Age = Manfred->Character.Age;   // Number (float)
Manfred->Character.Age = 42;

FString Motivation = Manfred->Character.Motivation; // Text (string)
Manfred->Character.Motivation = "Save the world";

bool IsStartLocation = LocationCell->LocationSettings.IsStartLocation; // Boolean (bool)
LocationCell->LocationSettings.IsStartLocation = false;
```

#### Slots

Slots are used to reference other articy objects, providing a way to link entities together. In Unreal, slot properties are automatically linked to the appropriate objects and can be cast to specific types as needed.

```cpp
UManiacManfredLocationSettings* LocationCell = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredLocationSettings>("Loc_Cell");
FArticyId StartDialogue = LocationCell->GetFeatureLocationSettings()->InitialDialog;
```

#### Strips and Calculated Strips

Strips allow you to reference multiple objects at once. In Unreal, strips are implemented as arrays, making it easy to iterate over the referenced objects.

```cpp
UManiacManfredCharacter* Manfred = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredCharacter>("Chr_Manfred");
for (UArticyObject* Obj : Manfred->Character.Likes)
{
	if (UArticyObject* Entity = Cast<UArticyObject>(Obj))
	{
		UE_LOG(LogTemp, Warning, TEXT("Manfred likes: %s"), *Entity->DisplayName.ToString());
	}
}
```

#### Drop-down Lists

Drop-down lists are implemented as enumerations in Unreal. This allows you to assign and compare values using the generated enum types.

```cpp
UManiacManfredConditional_Zone* TherapistZone = UArticyDatabase::Get(WorldContext)->GetObjectFromStringRepresentation<UManiacManfredConditional_Zone>("Zon_4B2E23F9");
TherapistZone->ZoneCondition.CursorIfConditionMatches = EMouseCursor::Hand;
```

#### Scripts

Scripts within templates, such as **Conditions** and **Instructions**, can be executed via Unreal’s C++ code. Template scripts are represented by `ArticyScriptCondition` or `ArticyScriptInstruction` depending on their type.

```cpp
UManiacManfredConditional_Zone* TherapistZone = UArticyDatabase::Get(WorldContext)->GetObjectFromStringRepresentation<UManiacManfredConditional_Zone>("Zon_4B2E23F9");

// Call a condition script
bool Result = TherapistZone->ZoneCondition.ClickCondition.CallScript();
if (Result)
{
    // If the condition returns true, call the instruction script
    TherapistZone->ZoneCondition.OnClickInstruction.CallScript();
}
```

---

### Accessing Object Features

While template data is bound to specific object types in articy:draft, the concept of **Features** allows you to reuse sets of properties across different templates. To interact with features, the Articy plugin in Unreal generates interfaces like `I<Project>ObjectWith<FeatureName>Feature`.

**Example: Accessing a feature via an interface:**

```cpp
UArticyObject* Obj = UArticyDatabase::Get(WorldContext)->GetObjectByName("Chr_Manfred");

if (IManiacManfredObjectWithCharacterFeature* CharacterFeature = Cast<IManiacManfredObjectWithCharacterFeature>(Obj))
{
    CharacterFeature->GetFeatureCharacter().Motivation = "Conquer the world";
}
```

This allows you to check if an object has a particular feature and access its properties without needing to know the exact object type.
