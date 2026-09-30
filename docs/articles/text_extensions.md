\page textExtensions Text extensions

In Unreal Engine, the **ArticyImporter** plugin automatically processes text through an advanced text processing system known as **Text Extensions**, mirroring the Unity version. This system allows you to use global variables, object properties, and other dynamic values inside text fields. You can insert special tokens into strings, which are then resolved at runtime to produce final, localized output. This guide covers how to use text extensions, including token formatting, variable substitution, and advanced use cases.

---

### Primer to Articy Text Extension

To utilize the text extension system, you add **tokens** inside your strings. A token is a placeholder that represents dynamic content, such as variables or object properties. The syntax is the same whether you're writing text directly in **articy:draft** or in Unreal Engine. Your string can contain as many tokens as you want, and tokens can even be nested.

#### Example:
```text
Hello, [Session.PlayerName]
```
In this example, `[Session.PlayerName]` is a token that will be resolved at runtime to the value of the `PlayerName` global variable.

> **Note:** If you want to use brackets in your literal text, escape them with a backslash. `He continued: "\[...\] it wasn't me!"` turns into `He continued: "[...] it wasn't me!"`; otherwise the text extension would try to resolve the part inside the brackets. Use `\\` for a literal backslash. Remember to escape the backslash again when writing the text inside a C++ string literal.

---

### Token Definition

A token is surrounded by brackets `[ ]` and typically follows this form:

```text
[(Source)(:Formatting)]
```

- **Source:** The data you want to display (e.g., global variables, object properties).
- **Formatting:** An optional field that allows custom formatting, particularly for numbers.

---

### Token Sources

Text extensions in Unreal allow you to pull information from various sources. Before looking at the sources, it helps to know how the different data types end up in your text:

- **Strings** are inserted as they are, with two exceptions: a string holding an object representation (as stored by `getObj()` in a script or in a string variable) inserts that object's localized **DisplayName**, and a string that is a valid localization key is localized first.
- **Numbers** are converted to text; the optional formatting gives you full control over their appearance.
- **Booleans** use the localized texts `<Source>.True` / `<Source>.False` or `VariableConstants.Boolean.True` / `False` when those keys exist in the string table, and `true` / `false` otherwise.
- **Enums** insert their display name; use the `D` format to get the numeric value.
- **Objects** insert their localized **DisplayName**.
- **Literals** can be used directly: `["Hello"]` inserts a string (localized if it is a key) and `[42:000]` formats a number.

#### Global Variables
You can reference global variables directly in your text by specifying their namespace and name.

**Example:**
```text
Hello [Session.PlayerName], you still only have [Inventory.GoldCount] gold left.
```

This will dynamically display the player's name and the amount of gold they currently have.

#### Objects
You can access objects and their properties and templates by using their ID or technical name. The general structure is:

```text
[(ID/TechnicalName)<(instanceId)>.(Properties)...]
```

**Examples:**
```text
[Chr_Manfred]
```
Inserts the display name of the character **Chr_Manfred**; the `.DisplayName` property can be omitted as it is the default for objects.

```text
[Chr_Manfred.Character.Motivation]
```
Accesses a property from the **Character** template feature associated with **Chr_Manfred**.

```text
[0x01000001000010C6.Text]
```
It is perfectly valid to use the object ID (hex or decimal) instead of the technical name.

```text
[MainCharacter.Character.Companion.Character.HP]
```
Here, `Companion` is a reference slot containing another character, and the token retrieves that companion's HP value.

```text
I don't talk to a filthy [[DialogueState.Counterpart].Basic.Class]
```
`DialogueState.Counterpart` is a string global variable holding an object representation. Because the string can be interpreted as an object, you can directly access its template and property.

#### Instances
When working with multiple instances (clones) of the same object, you can specify an **instance ID** to target a particular instance. Without it, the first instance is used.

**Example:**
```text
[Guard<15>.NPC.HP]
```
This targets instance 15 of the **Guard** object and retrieves the **HP** property.

#### Reference strips
Elements of a reference strip are accessed by index in angle brackets. `-1` selects the last element and `?` selects a random element.

**Example:**
```text
My first item is [Chr_Main.Inventory.ReferenceStrip<0>.DisplayName], my last one [Chr_Main.Inventory.ReferenceStrip<-1>].
```
The indices picked by `?` during the last resolve are available through `UArticyTextExtension::GetLastRandomResults()`.

#### Convenience Identifiers: `$Self` and `$Speaker`

Depending on the context object the text is resolved with, you can access that object (`$Self`) or, for dialogue fragments, its speaker (`$Speaker`). Both are object references and can be used accordingly. Texts read from articy objects are resolved with the object as context, so `$Self` is the object the text belongs to.

**Example:**
```text
Sorry, I only have [$Speaker.Inventory.Gold] gold left.
```

#### Type information: `$Type`

You can access meta information about your articy:draft types through the type system, for example to build UI labels from your templates. Address a type by name, or take the type of an object instance:

```text
[$Type.(ClassName).(Properties)...]
[(ObjectSource).$Type.(Properties)...]
```

A type resolves to its display name; `[$Type.Character]` and `[$Type.Character.DisplayName]` both insert the display name of the **Character** template. Its other members are `TechnicalName`, `CPPType`, `HasTemplate` and `IsEnum`, and the values of an enum type (`[$Type.CharacterClass.Bard:D]`).

A property is addressed as `Property` or, when it lives in a feature, as `Feature.Property`. On its own it inserts the property's display name, so `[$Type.Character.Basic.Backstory]` is a shortcut for `[$Type.Character.Basic.Backstory.DisplayName]`. Its other members are `TechnicalName`, `PropertyType` and `IsTemplateProperty`. (Constraint values such as `MaxValue` are not exported to the runtime type data yet.) When a property shares its name with a type member, such as `DisplayName`, the bare name means the type member and a longer path means the property: `[$Type.Character.DisplayName.PropertyType]`.

A token that names an unknown type, property or member is an invalid token (it resolves to an empty string by default, see the plugin settings below). Asking a property *value* for its type (`[Chr_Manfred.Character.Motivation.$Type]`) is not supported; use `[Chr_Manfred.$Type...]` for the object's type instead.

**Example:**
```text
[$Type.Character.Attributes.Strength.DisplayName]: [[Session.PlayerCharacter].Attributes.Strength]
```

---

### Token Formatting

Tokens can be formatted to modify the appearance of the resolved value. Formatting is applied after the source using a colon `:` followed by a format string that follows the **C# numeric format strings**, both the standard ones (`D`, `F`, `N`, `P`, `E`, `X`, each with an optional precision) and the custom ones (`0`, `#`, `.`, `,`, `%`, literals and `;` sections).

**Examples:**

- **Zero-Padding a Number:**
    ```text
    Highscore: [Session.Score:00000000]
    ```
    If the player's score is `42`, this will display: `Highscore: 00000042`.

- **Percentage Formatting:**
    ```text
    [Chr_Player.Combat.HitRating:P]
    ```
    If the hit rating is stored as a decimal (e.g., `0.42`), this will display `42%`.

- **Enum values:**
    ```text
    The enum value of [Chr_Player.Character.ClassEnum.DisplayName] is [Chr_Player.Character.ClassEnum:D]
    ```
    The first token shows the name of the class and the second its numeric value, e.g. `The enum value of Bard is 2`.

- **Masking Numbers:**
    ```text
    The secret code is [Riddles.NumberLock:###-###-###]
    ```
    If the number is `123456789`, it will display as `123-456-789`.

---

### Methods in Tokens

A method can take any number of parameters and always returns a string. Arguments can be quoted literals, other tokens and even other method calls.

#### `if` and `not`
You can conditionally add text depending on another token:
```text
Hello, how are you[if([Player.IsWearingPoliceUniform], ", Officer", "")]?
```
The first parameter is the condition (here another token resolving to a boolean). The second parameter is inserted when the condition is true, the optional third one otherwise. The `not` method works the same way but swaps the two branches.

---

### Working with Scripts in Tokens

Script properties on templates can be used in two ways:

```text
The script text is: [Something.Feature.ScriptProperty]
```
Without parentheses, the script source is inserted as text.

```text
The script value is: [Something.Feature.ScriptProperty()]
```
With parentheses the script is executed. A condition inserts its boolean result; an instruction is run and inserts nothing.

---

### Articy Text Extensions in Code

While text extensions work automatically for Articy object properties, you can also manually resolve tokens in your own strings. The `Resolve` methods take a context object (needed for `$Self` / `$Speaker`) and any number of positional parameters, which are inserted into `{0}`, `{1}`, ... before the tokens are resolved. If the input is a localization key, it is localized first.

```cpp
UArticyTextExtension* TextExtension = UArticyTextExtension::Get();

// Simple
FText Resolved = TextExtension->Resolve(this, FString(TEXT("Hi, [Chr_Manfred]")));
// output: "Hi, Manfred"

// With parameters, like String.Format
FText Cost = TextExtension->Resolve(this, FString(TEXT("You still need {0}g to buy this [Shop.SelectedItem].")), MissingGold);

// Parameters can even form part of a token
for (int32 Index = 1; Index <= PlayerCount; ++Index)
{
    FText Greeting = TextExtension->Resolve(this, FString(TEXT("Hello, [Session.Player{0}]")), Index);
}

// Nested tokens achieve the same
FText Turn = TextExtension->Resolve(this, FString(TEXT("[Session.Player[Session.CurrentPlayerIndex]], it's your turn!")));
```

`ArticyHelpers::ResolveText(Outer, &Text)` remains available as a shortcut for the `FText` overload.

#### Resolving a single token

`ResolveToken` resolves one token (without brackets) with the default logic and returns an empty string if it cannot be resolved. This is useful inside custom methods and `ResolveAdvance` callbacks:

```cpp
FString Score = UArticyTextExtension::Get()->ResolveToken(this, TEXT("Session.Score:000"));
```

---

### Custom Methods in Tokens

You can register your own methods and use them like the built-in ones. A method receives the token it was called from (`FArticyTextToken`: context object, the whole expression, the positional parameters, the token source and index) and its parsed arguments.

```cpp
UArticyTextExtension::Get()->AddUserMethod(TEXT("GetCurrentTime"), [](const FArticyTextToken& Token, const TArray<FString>& Args)
{
    return FDateTime::Now().ToString();
});

// Later, if the method is no longer needed
UArticyTextExtension::Get()->RemoveUserMethod(TEXT("GetCurrentTime"));
```

```text
The current time is [GetCurrentTime()].
The current time is [GetCurrentTime("now", [GetCurrentLocale()])].
```

The previous argument-only callback signature (`FString(const TArray<FString>&)`) is still accepted.

---

### Custom Token Resolving

For maximum flexibility, `ResolveAdvance` calls a callback for every token in the string. Return a value to resolve the token yourself, or an unset `TOptional` to fall back to the default resolving:

```cpp
FText Result = UArticyTextExtension::Get()->ResolveAdvance(this, Text, [](const FArticyTextToken& Token) -> TOptional<FString>
{
    if (Token.Token == TEXT("Custom.Value"))
    {
        return FString(TEXT("custom"));
    }
    return TOptional<FString>(); // default logic for everything else
});
```

---

### Plugin settings

Two settings in **Project Settings > Plugins > Articy X Importer** control the text extension:

- **Resolve strings** (default on): articy object texts (DisplayName, Text, MenuText, template texts) resolve their tokens automatically when read. Turn it off to get the raw strings and resolve them yourself with `UArticyTextExtension`.
- **Allow invalid tokens** (default on): a token that cannot be resolved is replaced by an empty string. When off, an error in any token makes the whole text resolve to its unresolved input instead.

---

The **Text Extensions** system in Unreal's ArticyImporter plugin is a powerful tool for embedding dynamic content into text fields. By using tokens, formatting options, and custom methods, you can create rich, dynamic, and localized text content that adapts to the game's state. Whether you're pulling from global variables, object properties, or even scripts, this system provides flexibility and ease of use in your narrative design.
