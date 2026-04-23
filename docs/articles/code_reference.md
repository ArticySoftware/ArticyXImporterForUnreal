# Code Reference

This page provides an overview of code for the **ArticyX Importer** plugin for **Unreal Engine**, covering key classes and functionalities. The **ArticyX Importer** enables seamless integration of **articy:draft** assets and data into **Unreal Engine** projects, automating various processes such as importing, asset generation, and scripting.

The full reference for the plugin's **C++ codebase** can be explored under [Classes](annotated.html). This documentation is structured similarly to other **Doxygen** projects, allowing easy navigation and search functionality within the class structure. Furthermore the full source code of the plugin is available on [GitHub](https://github.com/ArticySoftware/ArticyXImporterForUnreal).

---

## Key Class List

Here is a brief overview of the most important classes available in the **ArticyX Importer** plugin:

### 1. [**ArticyImporterHelpers**](@ref ArticyImporterHelpers)
Provides utility functions for importing and managing **articy:draft** assets in Unreal Engine.

### 2. [**AArticyFlowDebugger**](@ref AArticyFlowDebugger)
Handles the debugging of the **articy:draft** flow objects during gameplay.

### 3. [**ArticyEditor**](@ref ArticyEditor)
Defines the build rules and configurations for the **ArticyEditor** module, responsible for editor-related tasks.

### 4. [**ArticyLocalizerGenerator**](@ref ArticyLocalizerGenerator)
Utility class responsible for generating **localization code** based on **articy:draft** import data.

### 5. [**ArticyTypeGenerator**](@ref ArticyTypeGenerator)
Generates the necessary type structures from **articy:draft** data, simplifying access to imported types in Unreal.

### 6. [**BuildToolParser**](@ref BuildToolParser)
Parses build tool files to manage runtime references for the **articy:draft** runtime environment.

### 7. [**CodeFileGenerator**](@ref CodeFileGenerator)
Generates and manages code files for the plugin, including utilities for generating code lines, comments, and more.

### 8. [**CodeGenerator**](@ref CodeGenerator)
Handles the generation, compilation, and asset creation process for data imported from **articy:draft**.

### 9. [**DatabaseGenerator**](@ref DatabaseGenerator)
Generates project-specific database code and assets for **articy:draft** imports, ensuring efficient data access.

### 10. [**ExpressoScriptsGenerator**](@ref ExpressoScriptsGenerator)
Responsible for generating expresso scripts and their associated methods for **articy:draft** import data.

### 11. [**FArticyEnumValueInfo**](@ref FArticyEnumValueInfo)
Contains detailed information about enum values used in the **articy:draft** runtime.

### 12. [**FArticyPropertyInfo**](@ref FArticyPropertyInfo)
Holds metadata about **articy:draft** properties, including their types and constraints.

### 13. [**FArticyType**](@ref FArticyType)
Represents a specific type from **articy:draft** data, including information about properties, features, and enum values.

### 14. [**FArticyRect**](@ref FArticyRect)
A utility class for managing rectangular bounds, commonly used in **articy:draft** location objects.

### 15. [**FArticyEditorConsoleCommands**](@ref FArticyEditorConsoleCommands)
Provides console commands to reimport **articy:draft** data and manage other editor-level operations directly via the Unreal Engine console.

### 16. [**UArticyFlowPlayer**](@ref UArticyFlowPlayer)
Manages the flow of gameplay logic, executing dialogues, scene transitions, and other branching logic based on **articy:draft** flow objects.

### 17. [**UArticyGlobalVariables**](@ref UArticyGlobalVariables)
Stores global variables from **articy:draft**, allowing them to be accessed and modified during gameplay.

### 18. [**UArticyObjectWithTransform**](@ref UArticyObjectWithTransform)
Represents objects with positional data in **articy:draft**, such as objects in a scene or level.

### 19. [**UArticyTextExtension**](@ref UArticyTextExtension)
Handles advanced text processing and token replacement using **articy:draft** text objects.

### 20. [**UArticyCondition**](@ref UArticyCondition)
Represents conditional expressions in **articy:draft**, used for evaluating gameplay logic and interactions.

### 21. [**UArticyImportCommandlet**](@ref UArticyImportCommandlet)
A commandlet used for automating the import process in a build server or scripting environment. It supports flags for forcing reimports or regenerating assets.

---

## Usage of the Documentation

The **Doxygen** project includes comprehensive documentation for every class, struct, and interface, as well as detailed descriptions for all methods and members. You can explore:

- **Inheritance Diagrams**: For understanding class hierarchies and relationships.
- **Class Index**: To quickly find classes and interfaces.
- **Function Documentation**: Includes descriptions, parameter lists, and return types for each method.

## Navigating the Class Structure

1. **Browse the Class List**: Start by exploring the **Class List** on the left navigation panel of the **Doxygen** documentation.
2. **Use the Search Function**: Use the built-in search function to quickly find specific classes or methods.
3. **Inheritance Diagrams**: View the relationships between different classes and their parents or children, helping you understand the structure of the plugin code.
