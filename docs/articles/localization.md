# Localization

The **ArticyImporter** plugin for Unreal Engine processes localization data from **articy:draft X** and integrates it with Unreal's localization system. This allows developers to access and manage localized content efficiently in both C++ and Blueprint. Localization data is automatically stored in **String Tables** as CSV files, located within `ArticyContent/Generated`. These string tables are loaded at runtime, and Unreal’s standard localization tools can be used to manage them. Additionally, the generated CSV files must be included in the built game package to ensure proper functionality in shipped products.

---

@tabs{
@tab{ C++ | cpp | 

#### Accessing Localized Properties

In Unreal, when accessing a property that supports localization, the **ArticyImporter** plugin automatically loads the appropriate localized text based on the current language setting.

**Example: Accessing a localized property**

```cpp
UManiacManfredCharacter* Manfred = UArticyDatabase::Get(WorldContext)->GetObject<UManiacManfredCharacter>("Chr_Manfred");
UE_LOG(LogTemp, Warning, TEXT("Localized Display Name: %s"), *Manfred->GetDisplayName().ToString());
```

In this case, the **DisplayName** property is localized, and the appropriate translation is fetched and displayed based on the active language.

#### Custom Localized Properties

If you have custom properties that need localization, the plugin provides a function, `GetPropertyText`, to retrieve localized text values for properties defined in **articy:draft**.

**Example: Using `GetPropertyText`**

```cpp
FText LocalizedVoiceActor = Manfred->GetPropertyText(Manfred->VoiceActor);
UE_LOG(LogTemp, Warning, TEXT("Localized Voice Actor: %s"), *LocalizedVoiceActor.ToString());
```

This function retrieves the localized text for a specific property, such as the **VoiceActor** property in the example above.

Additionally, helper functions for localized properties are generated in the `<Project>ArticyTypes.h` file. For instance:

```cpp
FText GetVoiceActor() { return GetPropertyText(VoiceActor); }
```

This allows you to easily access localized properties using predefined functions.

#### Changing the Active Language

To change the language in Unreal, you can use the built-in Unreal localization system to change the active language of the game. The **String Table** system integrated by **ArticyImporter** will automatically use the correct language for all localized properties.

#### Including String Tables in the Game Package

Ensure that the CSV files generated for string tables are included in your final game package. These are located in the `ArticyContent/Generated` folder, and they must be loaded at runtime for localization to work properly in the shipped game.

}
@tab{ Blueprint | uebp |

The **articy:draft** plugin also supports localization in **Blueprints**, making it easy for non-programmers to work with localized content.

#### Accessing Localized Properties in Blueprint

Localized properties can be accessed directly within Blueprints. For example, to access the localized **DisplayName** of an entity:

1. Drag and drop the **Articy Object** node (e.g., **UManiacManfredCharacter**).
2. Use the **Get Display Name (Localized)** function to retrieve the localized text.

#### Custom Localized Properties in Blueprint

You can also access custom localized properties via Blueprints using the generated functions from the **ArticyImporter** plugin.

1. Select the object.
2. Use the **Get Property (Localized)** node to retrieve the localized version of a custom property (such as **VoiceActor**).

#### Changing the Active Language in Blueprint

You can change the language of the game in Blueprint using the **Set Current Culture** node, which is part of Unreal's localization system. Once the language is changed, all localized properties will automatically be updated to reflect the new language.

}
}

---

### Example: Using the String Table System

Here is a step-by-step process for working with localization in Unreal, specifically using **articy:draft** data:

1. **Access Localized Properties**: Use the `GetPropertyText` function (in C++) or **Get Property (Localized)** node (in Blueprint) to retrieve the localized text.
2. **Change Language**: Use the **Set Current Culture** node (in Blueprint) or Unreal's localization functions (in C++) to change the active language.
3. **Ensure String Tables are Included**: Verify that the generated CSV files from `ArticyContent/Generated` are packaged with your game.

---

In Unreal Engine, localization with **articy:draft** is seamless, with the plugin generating **String Tables** to manage localized text. Standard Unreal localization tools are used to change and manage languages, while custom localized properties can be easily accessed using the **GetPropertyText** function in C++ or **Get Property (Localized)** nodes in Blueprint. Always ensure that the generated string tables are included in your packaged game to provide proper localization in your final build.
