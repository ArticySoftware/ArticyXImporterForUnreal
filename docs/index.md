# Overview {#index}

This page provides a quick start on how to use the ArticyXImporter plugin for Unreal Engine, introduces its core features, and gives an overview of how to work with articy:draft X data inside Unreal projects.

The purpose of this plugin is to easily import and integrate **articy:draft X** content into Unreal Engine, providing access to data through both C++ and Blueprint interfaces. It simplifies using game data, dialogues, and branching narratives in your Unreal projects.

### Features

- **C++ and Blueprint API:** Access your articy:draft X data through both code and visual scripting.
- **Flow Player for Dialogues:** Manage narrative flows and dialogues set up in articy:draft X with ease.
- **Automatic Import:** Quickly bring articy:draft X data into Unreal Engine.
- **Localization Support:** Handle different language versions using Unreal’s built-in localization system.

### Quick Start

1. **Set up your Unreal project:**
   Ensure your Unreal project supports C++. If your project is Blueprint-only, convert it by adding a simple C++ class, like `MyActor`. After creating the class, your project will now support both C++ and Blueprints.

2. **Install the plugin:**
   - You can either install the plugin via GitHub or the Unreal Engine Marketplace. 
   - Once installed, enable it by going to `Edit -> Plugins` in Unreal and enabling `ArticyXImporter`. Restart the editor when prompted.

3. **Export your articy:draft X data:**
   Open your articy:draft X project and select the **Unreal Engine export** option. Export the data to your Unreal project’s `Content` folder.

4. **Import your data:**
   The plugin will detect the export and prompt you to import the data. You can opt for a full import, import changes, or regenerate assets as necessary.

5. **Access your data:**
   Use **ArticyRef** to reference your articy:draft X objects. The **Articy Asset Picker** helps select and manage these references easily.

### Articy Flow Player

The **Articy Flow Player** is an essential tool for traversing your dialogue and narrative flows. It handles branching paths, evaluates conditions, and manages callbacks to your game code to control the player’s narrative choices. 

You can learn more about setting up dialogues and managing branching stories in the [Dialogues and Flow Traversal](Dialogues-and-Flow-Traversal) section.

### Global Variables and Scripting

Global variables imported from articy:draft X can be used to track game states and progress, while **Custom Script Methods** allow you to define and execute complex game logic. For detailed usage, see the [Scripting and How to Use It](Scripting-and-How-to-Use-It) section.

### Object Handling

To access and manipulate objects, the **ArticyRef** struct provides an easy way to interact with entities, dialogues, and templates from your articy:draft X project. Check out the [Basic Object Handling](tutorials/basic_object_handling.md) and [Object Handling with ArticyRef](Object-Handling-with-ArticyRef) sections for more details.

### Localization and Text Extensions

For handling different languages and text formats, you can use Unreal’s localization system along with text extensions from articy:draft X. More information is available in the [Localization](Localization) and [Text Extensions](Text-Extensions) sections.

---

This welcome guide is designed to get you started quickly with the ArticyXImporter plugin and help you explore its integration features for Unreal Engine.
