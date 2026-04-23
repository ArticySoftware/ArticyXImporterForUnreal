\page textExtensions Text extensions

In Unreal Engine, the **ArticyImporter** plugin automatically processes text through an advanced text processing system known as **Text Extensions**, similar to the Unity version. This system allows you to use global variables, object properties, and other dynamic values inside text fields. You can insert special tokens into strings, which are then resolved at runtime to produce final, localized output. This guide covers how to use text extensions, including token formatting, variable substitution, and advanced use cases.

---

### Primer to Articy Text Extension

To utilize the text extension system, you add **tokens** inside your strings. A token is a placeholder that represents dynamic content, such as variables or object properties. The syntax is the same whether you're writing text directly in **articy:draft** or in Unreal Engine.

#### Example:
```text
Hello, [Session.PlayerName]
```
In this example, `[Session.PlayerName]` is a token that will be resolved at runtime to the value of the `PlayerName` global variable.

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

Text extensions in Unreal allow you to pull information from various sources. These sources include global variables, object properties, templates, and more.

#### Global Variables
You can reference global variables directly in your text by specifying their namespace and name.

**Example:**
```text
Hello [Session.PlayerName], you still only have [Inventory.GoldCount] gold left.
```

This will dynamically display the player's name and the amount of gold they currently have.

#### Objects
You can also access properties of objects and templates by using their ID or technical name.

**Examples:**
```text
[Chr_Manfred.DisplayName]
```
Displays the display name of the character **Chr_Manfred**.

```text
[Chr_Manfred.Character.Motivation]
```
Accesses a property from the **Character** template associated with **Chr_Manfred**.

#### Instances
When working with multiple instances of the same object, you can specify an **instance ID** to target a particular instance.

**Example:**
```text
[Guard<15>.NPC.HP]
```
This targets the 15th instance of the **Guard** object and retrieves the **HP** property.

---

### Token Formatting

Tokens can be formatted to modify the appearance of the resolved value. This is especially useful for numbers and dates. Formatting is applied after the source using a colon `:` followed by a format string, similar to **C# string formatting** rules.

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

- **Masking Numbers:**
    ```text
    The secret code is [Riddles.NumberLock:###-###-###]
    ```
    If the number is `123456789`, it will display as `123-456-789`.

---

### Convenience Identifiers: `$Self` and `$Speaker`

Depending on the context of the text, you can access the current object (`$Self`) or the speaker in a dialogue fragment (`$Speaker`).

**Example:**
```text
Sorry, I only have [$Speaker.Inventory.Gold] gold left.
```
In this example, the token references the speaker's gold count in the inventory.

---

### Advanced Token Usage

You can access properties across multiple related objects, access strips (lists of references), and even nest tokens within other tokens.

#### Nested Token Example:
```text
[MainCharacter.Character.Companion.Character.HP]
```
Here, `Companion` is a reference slot containing another character, and the token retrieves that companion's HP value.

---

### Methods in Tokens

You can conditionally modify the output of your text using built-in methods like `if` and `not`.

#### Conditional Text Example:
```text
Hello, how are you[if([Player.IsWearingPoliceUniform], ", Officer", "")]?
```
This checks if the player is wearing a police uniform. If they are, the text adds ", Officer"; otherwise, it adds nothing.

The `not` method works similarly but inverts the conditions.

---

### Working with Scripts in Tokens

Script properties in Articy can also be accessed and executed within tokens. 

#### Script Access Example:
```text
The script value is: [Something.Feature.ScriptProperty()]
```
This executes the script and returns the result. For condition scripts, this will return a boolean.

---

### Articy Text Extensions in Code

While text extensions work automatically for Articy object properties, you can also manually resolve tokens in your own strings using the `ResolveText` function. This is useful when building dynamic text outside of Articy-generated objects.

#### Manual Token Resolution Example (C++):
```cpp
FText CustomText = FText::FromString("Hello, [Session.PlayerName], you have [Inventory.GoldCount] gold.");
FText ResolvedText = ArticyHelpers::ResolveText(this, &CustomText);
UE_LOG(LogTemp, Warning, TEXT("Resolved Text: %s"), *ResolvedText.ToString());
```

---

### Custom Methods in Tokens

You can create your own custom methods to use inside tokens. Custom methods are registered with the text extension system and can be used like built-in methods.

#### Registering a Custom Method (C++):
```cpp
static FString GetCurrentTime(const TArray<FString>& Args)
{
    return FDateTime::Now().ToString();
}

// Register the method
UArticyTextExtension::AddUserMethod("GetCurrentTime", &GetCurrentTime);
```

#### Using the Custom Method:
```text
The current time is [GetCurrentTime()].
```

---

The **Text Extensions** system in Unreal's ArticyImporter plugin is a powerful tool for embedding dynamic content into text fields. By using tokens, formatting options, and custom methods, you can create rich, dynamic, and localized text content that adapts to the game's state. Whether you're pulling from global variables, object properties, or even scripts, this system provides flexibility and ease of use in your narrative design.
