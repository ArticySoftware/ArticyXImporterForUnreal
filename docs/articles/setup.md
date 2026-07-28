\page setup Setup

### Project setup

A couple of steps are needed to get the importer up and running. The Unreal project must be C++ compatible, therefore please ensure that the required tools are installed, such as Visual Studio for Windows or XCode for Mac.

To find out more about how to set up Visual Studio, click [here](https://docs.unrealengine.com/en-US/Programming/Development/VisualStudioSetup/index.html).

The following options are available when first using the importer:

#### Create a new project

To create a new project, select a project template of your choice and in the next step choose to use C++. You will still be able to use Blueprints, however, your project will immediately support C++!

<p align="center">
  <img height="400" src="https://www.articy.com/articy-importer/unreal/adx/create_new_cpp_project.png">
</p>

After you have created a new project, close the Unreal editor for now.

#### For already existing Blueprint-only projects

Projects that existed prior to using the importer and used only Blueprints can be converted to C++ projects by adding any C++ class that inherits from UObject to the project. It is important that an option <b>other</b> than None gets selected. As an example, we choose the Actor class.

<p align="center">
  <img height="400" src="https://www.articy.com/articy-importer/unreal/adx/create_new_cpp_class.jpg">
</p>

<p align="center">
  <img height="400" src="https://www.articy.com/articy-importer/unreal/adx/select_actor_as_parent_class.png">
</p>

Click Next, name the class "MyActor" and finish the setup. Unreal Engine should now compile the "MyActor" class. After having compiled, your project now also works with C++.

### How to continue

Get a quick start by reading the [Getting Started](@ref gettingStarted) guide or read about specific topics to dive deep:

- Use the flow player to [handle dialogues and flow traversal](@ref dialoguesFlowTraversal)
- Work with your [Articy objects in the engine](@ref basicObjectHandling)
- Learn how to use [Templates](@ref objectTemplates)
- Discover how to optimize your import flow using [selective import](@ref selectiveImport)
- ...or have a look at the list of articles on the left-hand side to explore more possibilities
