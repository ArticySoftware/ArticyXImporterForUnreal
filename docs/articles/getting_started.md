\page gettingStarted Getting started

After your project is setup with the plugin installed, the next step is to get data from articy:draft X into the engine and do something with it. This article will get you started with using the plugin.

### Export project from articy:draft X

Now that the importer is running, you are ready to export your data from articy:draft X.

Open your articy:draft X project and open the export window. Here you will find the Unreal Engine export. Please note that the Unreal export uses [Rulesets](https://www.articy.com/help/Exports_Rulesets.html) to choose what and how to export.

When exporting, chose your Unreal projects **Content** folder as the target for the `.articyue` export file.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/export_options.png">
</p>

### Import into Unreal

The first time the ArticyXImporter plugin finds content to import, the plugin will automatically try to find an `ArticyRuntime` reference inside the project's Unreal build tool files. If the plugin can't find any reference, it will show the following messagebox to add it automatically :

![](ArticyRuntimeRef_AutoAdd.png)

- **"Yes"** will automatically add the ArticyRuntime reference inside the Unreal build file.
- **"No"** will continue import (no modification to Unreal build files).
- **"Cancel"** will abort import process.

> [!NOTE]
> The automatic verification process can be disabled inside the project settings > Plugins > Articy X Importer (uncheck "Verify ArticyRuntime reference inside Unreal Build tools").

After every export, going back to Unreal will trigger the ArticyXImporter plugin to automatically parse the new file and show a prompt to import the changes. While this option is generally robust, there are certain cases in which more control over the import process is required.

For greater control over your imports, use the Articy X Importer Menu. It can be accessed through the Articy X Importer button on the Level Toolbar (UE4) or through the Settings menu (UE5).

**Unreal Engine 4**
![](https://www.articy.com/articy-importer/unreal/adx/articywindow.png)

**Unreal Engine 5**
![](ImporterButtonUE5.png)

#### Importer Modes

- **Full Reimport**: This option will always regenerate code and compile it, and afterward generate the articy assets
- **Import Changes**: This option will only regenerate code and compile it if necessary, but will always regenerate assets. This is generally faster than a full reimport and is the same as clicking on 'Import' on the prompt Unreal shows you when you've exported.
- **Regenerate Assets**: This option will only regenerate the articy assets based on the current import data asset and compiled code.

> [!NOTE] 
> After **upgrading the importer to a new version**, restart the Unreal editor before the first **Play** following an import. Importing recompiles and hot-reloads the project's generated code in-session, and after a version change that hot reload can crash on Play. Restarting loads the regenerated code cleanly. The importer will show a reminder when it detects an upgrade.

### Using the API

Now that the importer is installed and your project data is imported you can start working on your project.
Here are some quick tips on what to look for so you won't go in completely blind:

#### Assigning an ArticyRef

To get access to your objects, an ArticyRef struct is used.
An ArticyRef variable can be assigned using the custom asset picker.

![Articy Asset Picker](https://www.articy.com/articy-importer/unreal/adx/assetpicker.gif)

Learn more about using [ArticyRef in the dedicated article](@ref objectHandlingArticyRef).

#### Getting the object

Once you have assigned your ArticyRef variable, you can then get access to the object by calling the 'Get Object' function. The returned object is an ArticyObject that requires casting or alternatively, the 'Get Object' function will cast it for you to the specified class.

A simple setup that prints the display name of the selected ArticyObject is shown below. Please note that the 'Get Display Name' function is an interface call, meaning that you don't need to cast if the object has a display name. If the object does not have a display name, such as a dialogue fragment, an empty text is returned.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/articyref_access.png">
</p>

There are many other ways to access your objects, check this screenshot for a blueprint sample code showing you how to access an object by id/technical or directly clone it. The ArticyDatabase is the central object that lets you access all imported articy data. Even the 'Get Object' function above makes use of the ArticyDatabase.

Also, make sure to cast the object to the desired type to get access to its properties and its template.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/get_object.png">
</p>

Find out more about object handling in the [basic object handling article](@ref basicObjectHandling).

#### Accessing properties

Most of the time if you want to access the properties of an object you queried from the database or got passed by the flow player callbacks (see below) you need to
cast the object to the correct type first.

If you have an object without a template the type to cast into is easy.
Every built-in class is named as the object in articy:draft X with your project name as a prefix. Let's say your project is named `ManiacManfred` so you will find `ManiacManfredFlowFragment`, `ManiacManfredDialogueFragment`, `ManiacManfredLocation`, `ManiacManfredEntity` and a lot more.
All those respective objects have their expected properties, so you will find the `Speaker` property inside the `ManiacManfredDialogueFragment` object, etc.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/base_object_property.png">
</p>

> [!TIP]
> You will also find classes with the `Articy` prefix. Those are the base classes for the generated classes and casting into them works almost the same. This would allow you to create code that is reusable
> independent of any imported project.

Dealing with templates is a bit more complicated. The first thing to understand is that **all your articy:draft X templates are new types** inside Unreal.

The name of your template types also follows a similar structure as the one mentioned before, but utilizing the Templates technical name: `<ProjectName><TemplateTechnicalName>`. So if your project is called *ManiacManfred* and your templates technical name is *Conditional_Zone* your correct type would be called `ManiacManfredConditional_Zone`.
Its also worth mentioning that even if it is a new type, it is still derived from the base type with all its properties.

Accessing is easy once you have cast the object into the correct type, just drag a connection out of the node and search for the type to see all its properties.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/base_object_propertylist.png">
</p>

For templates, it works the same way, but you will also find fields for every feature inside your template, so in the case of the `Conditional_Zone` template, there is a `ZoneCondition` field for the feature with the same name.

> [!NOTE]
> It is possible that the **context sensitive** search does not properly work at this point in blueprint. When you disable it, you should be able to see all the fields inside your object.

Some properties are a bit more complex like reference strips and scripts:

* Scripts contain methods to `Evaluate` the underlying script. You can also access the `Expression` which is the original script in text form.
* ReferenceStrips and QueryStrips are just arrays.
* ReferenceSlots, `Speaker`, `Asset` inside the `PreviewImage` etc. are of type `ArticyId`, which can be plugged into `GetObject`.

So to reiterate:

1. Get the object and cast it to the appropriate type.
2. Access the property/feature.
3. If it is a feature you access now the property inside the feature. <br/>
   3a. If it is a script method, you can execute it via the `Evaluate` method.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/object_with_script.png">
</p>

Get to know about using [templates in the corresponding article](@ref objectTemplates).

#### Articy Flow Player

The flow player is used as an automatic traversal engine.
To set it up you add the Flow player actor component to one of your actors:

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/flow_player_component.png">
</p>

Now you can customize the flow player by setting the necessary options in the Setup section of the details panel. Most interestingly are **Pause On** and **Start On**. The StartOn attribute can also be set dynamically via code before traversing through the dialogue.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/flowplayer_attributes.png">
</p>

If you scroll down you will find the components event section. Here you probably want to add events for **On Player Paused** and **On Branches Updated**

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/component_events.png">
</p>

Adding those will create new event nodes in the graph of your current actor and allow you to implement your custom logic.

To quickly reiterate how to use those: **On Player Paused** is called with the current paused object of the flow traversal, the current spoken dialogue fragment for example; and **On Branches Updated** to create the user choices for the current pause.

Here is an example blueprint implementation for both methods

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/component_basic_event_implementation.png">
</p>

The `ShowPausedObject` method is to display the current pause on the UI. Here is the implementation of that method.


<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/show_paused_text.png">
</p>

`spokenText` is bound to a UI text block.


And the **On Branches Updated** is used to create a vertical list of buttons. How to create those buttons, creating the layout etc. is out of scope of this quick guide but it is important that you
store the branch in every button. When you instantiate the button you should pass in the reference used in the for-loop and when the button is clicked you use that branch as the index for the flow player so it knows where to continue.

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/clicked_branch.png">
</p>

Learn more about the flow player in the [dialogues and flow traversal article](@ref dialoguesFlowTraversal).
