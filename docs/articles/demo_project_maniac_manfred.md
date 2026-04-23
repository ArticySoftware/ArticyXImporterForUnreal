# Demo project Maniac Manfred

## Introduction

The **Maniac Manfred** demo project showcases how **articy:draft** and **Unreal Engine** work together to create a playable **Point-and-Click Adventure** game. Originally designed as a reference project, this demo provides a practical guide to using **articy:draft** for planning, designing, and creating a game, and using **Unreal Engine** to bring it to life through the **ArticyImporter** plugin.

While the project demonstrates many of the typical features of an adventure game, it is not a fully optimized or polished product but rather a proof of concept and learning resource.

### Key Features of Maniac Manfred:
- **Simple Point-and-Click mechanics**
- **Inventory System** with item combination
- **Interactive branching dialogue system**
- **Multiple scenes**
- **Moral system** with different endings
- **Localization support**

---

## Setup and Installation

To get started, ensure that you have the necessary tools and files:

- **articy:draft X**: For making changes to the project and exporting them.
- **Unreal Engine 5.0 or higher**: The game engine to run the project.
- **Maniac Manfred articy:draft X demo project.**
- **Maniac Manfred Unreal demo project**: Available on the [GitHub repository](https://github.com/ArticySoftware/ArticyXDemoProjectForUnreal-ManiacManfred).

You only need the **articy:draft** Maniac Manfred project if you want to make changes and re-export data into Unreal.

---

## Project Overview

The **Maniac Manfred** demo's main goal is to demonstrate how to use **articy:draft** to plan, design, and create a game, and how to integrate this into **Unreal Engine** using the **ArticyImporter** plugin. 

### Game Features:
- **Simple Point-and-Click gameplay**: The player navigates and interacts with objects in various scenes.
- **Branching dialogue**: The player’s choices influence the outcome of conversations and events.
- **Inventory system**: Players can collect items, combine them, and use them to solve puzzles.
- **Multiple scenes**: The game spans several locations with different interactive elements.
- **Moral system and multiple endings**: Player choices influence the game's outcome.
- **Localization**: Support for multiple languages through the Articy system.

---

## Locations and Clickable Zones

### Locations in articy:draft

Each **scene** in **Maniac Manfred** is represented as a **Location** in **articy:draft**. The designer used **zones** within each location to create clickable areas. These zones define the interactive parts of the scene, such as doors, characters, or items. For instance, the first scene places the protagonist, Manfred, in a cell, where clickable zones trigger interactions with the environment.

**Links** in the scene allow the designer to reference other objects or scenes, such as linking a door to a hallway.

### InteractionZone Template

Each **clickable zone** uses a custom template called **InteractionZone**, which defines the behavior of the zone. This template includes:

- **Click Condition**: A script that determines if the player is allowed to click on the zone.
- **On Click Instruction**: A script that executes when the zone is clicked.
- **Item to Interact With**: Optional reference to an item that can be used with the zone.
- **Interaction Condition**: A script that checks if the item can be used on the zone.
- **Link if Item is Valid/Invalid**: References to objects triggered by valid or invalid item usage (e.g., dialogues).

### Location Generation in Unreal

Location generation in **Unreal Engine** is handled by the `ULocationGenerator` class, written in **C++**. This class automates the process of creating game objects based on **articy:draft** locations. It populates the scene by generating actors for each zone, attaching the necessary components for interactivity, and applying any custom logic defined in the template.

#### Location Generation Code Overview

- **GetAllChildrenRecursive**: Recursively retrieves all children of an actor in the scene.
- **GetNameForActor**: Determines the appropriate name for the actor based on the **articy:draft** object it represents.
- **GenerateLocation**: The main method for generating the location, which:
  - Clears previously generated objects in the scene.
  - Calculates the bounds of the 2D elements to be created.
  - Recursively creates game objects for each element in the location.
  
- **GenerateChildren**: Recursively generates child objects, attaches components (e.g., colliders for clickable zones), and sets the object's position and scale based on **articy:draft** data.
- **SetSpritePolygonCollider**: Sets the polygon collider for a sprite based on the object's vertices.

---

## Dialogues and Scene Transitions

### Dialogues

Dialogues are implemented using **Unreal Engine’s** UI system with panels and text elements. The dialogue system allows for **branching conversations**, where player choices influence the direction of the dialogue.

The **SceneHandler** class handles dialogue and scene transitions. When a player clicks on an object, the **SceneHandler** checks the type of object (e.g., location, item, or dialogue) and responds accordingly:

- If the object is a location, a new scene is loaded.
- If the object is an item, it is added to the player’s inventory.
- If the object is a dialogue, the system opens the dialogue UI and presents the player with dialogue choices.

### Scene Transitions

When transitioning between scenes, the **SceneHandler** triggers a visual transition and loads the new scene using the technical name of the location. The scene is loaded after a short delay to allow for transition animations to complete.

---

## Branches

Each branching dialogue option is displayed as a button in a **vertical list**. The **SceneHandler** updates the available branches each time the dialogue advances, clearing old buttons and creating new ones. A custom script, **BranchChoice**, is responsible for managing each dialogue option.

When a player selects a branch, it triggers the **ArticyFlowPlayer** to advance the story, updating the dialogue or progressing the scene.

---

## Inventory Management

The **inventory system** allows players to collect items, combine them, and use them in the game world. Each item is linked to a **global variable** that tracks whether the player possesses the item.

### Item Combination

Each item in **articy:draft** has a reference to a **Valid Combination** object. If the player tries to combine two items, the system checks if they are valid combinations. If successful, the two items are consumed, and a new item is added to the inventory. If the combination is invalid, a dialogue may be triggered to inform the player.

---

## Image Elements

Scenes in **Maniac Manfred** often include static **background images** and dynamic **scene elements**. The template for these image objects includes a **display condition** script that controls whether the image is visible based on the game state.

---

## Creating Scenes from Locations

The **LocationGenerator** automatically creates scenes based on **articy:draft** locations. Using the `ULocationGenerator` class, a developer can select a location, press a button, and have the scene automatically generated with all the appropriate game objects, colliders, and components.

---

The **Maniac Manfred** demo project provides a comprehensive example of how to integrate **articy:draft** with **Unreal Engine** to create a Point-and-Click Adventure game. By leveraging the **ArticyImporter** plugin, you can streamline the process of creating locations, dialogues, and interactive elements in your game, enabling a more efficient development workflow. While this demo project is designed to be simple, the techniques used here can be adapted to more complex projects with similar needs.
