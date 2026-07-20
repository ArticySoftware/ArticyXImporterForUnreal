\page customizations Customizations

### ArticyId Widget Customization
The ArticyId Widget customization is a system to let you add custom widgets to any ArticyId or ArticyRef property in the Unreal Engine editor without having to modify the plugin code. The Articy button that opens up the currently selected object inside articy:draft X itself is also implemented using the same system.

The system also allows you to test the associated object for its data, or to only use your custom widget for specific types of articy objects. For example, all objects that have a "Quest giver" feature in articy:draft X can show a button inside Unreal that will open up the asset in which the associated quest is contained.

To create a customization, there are three steps:
1. Create a Customization class inheriting from IArticyIdPropertyWidgetCustomization and override its functions. This class is responsible for actually creating the FExtender and the widget for the customization.
2. Create a Customization Factory class inheriting from IArticyIdPropertyWidgetCustomizationFactory and override its functions. The SupportsType function gives you the object that is being considered for customization as a parameter, so you can retrieve its data and decide whether the factory supports the articy object or not.
3. Register the Customization Factory with the ArticyCustomizationManager that resides in the FArticyEditorModule using a FOnCreateArticyIdPropertyWidgetCustomizationFactory object, which is going to instantiate a factory internally.

Sample code can be found in the following files inside the ArticyEditor module, which demonstrates how the articy button itself was added:
- "DefaultArticyIdPropertyWidgetCustomizations" (Customization + Factory)
- "ArticyEditorModule" (Registering of the factory via FOnCreateArticyIdPropertyWidgetCustomizationFactory object)

Keep in mind that this customization needs to be stripped from a packaged build, so this should only be done in an editor module.
The editor module needs to have "ArticyEditor" listed as a dependency.
Additionally, to get access to the customization manager from outside the FArticyEditorModule, use the following code:
```cpp
FArticyEditorModule::Get().GetCustomizationManager()->RegisterArticyIdPropertyWidgetCustomizationFactory...
```
