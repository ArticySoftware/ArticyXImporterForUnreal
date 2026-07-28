<p align="center">
  <img height="344" src="https://www.articy.com/articy-importer/unreal/adx/ad_importer_large.png">
</p>

[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)

# Articy X Importer for Unreal Engine

Interactive stories are complex by nature and require powerful narrative design tools. Use [articy:draft X](https://articy.com) as a visual database to manage all your storylines, characters, and variables in one single source of truth. From macro to micro, the nested Flow View helps you build coherent stories from start to finish—even when handling a myriad of player choices.

> [!TIP]
> Learn more about what you can do with articy:draft X [on our website](https://articy.com).

The Unreal Engine importer allows integrating articy:draft X content into Unreal projects and provides a simple C++ and blueprint interface to work with the data. We release this importer as a GitHub open source project that will provide a substantial head start into incorporating articy:draft X data into Unreal projects while still allowing enough flexibility to adjust the importer to personal needs. 

While full support is not guaranteed for this product, we're constantly working on improvements and would love to hear your suggestions. Feel free to forward us your ideas or even better directly [contribute](CONTRIBUTING.md) to the development of the importer.

# Features

This importer provides a working foundation for integrating articy:draft X content into Unreal Engine projects. You can expect the following features including but not limited to:

 * Everything accessible via **C++** and **Blueprint**
 * **Flow player** for automatic configurable flow traversal as an actor component
 * Automatic import
 * Articy Asset Picker for easy lookup and selection of your articy data
 * Database with your project data. *Excluding Journeys, Settings, Template constraints* 
 * Use of Unreal Engine's localization
 * Selective package import for fast incremental updates from articy:draft X

# Setup

## Quick start

1. Set up your Unreal project to support C++
2. Install the plugin via Fab or place it in your project's `Plugins` directory
3. Enable the plugin in your project

## Prepare your Unreal project

Ensure your Unreal project supports C++. If your project is Blueprint-only, convert it by adding a simple C++ class, like `MyActor`. After creating the class, your project will now support both C++ and Blueprints. For further assistance [refer to the docs](https://articysoftware.github.io/ArticyXImporterForUnreal/setup.html).

## Downloading the plugin

You can decide to get the plugin at [Fab](https://www.fab.com/listings/e9edb068-c115-4fb0-a7f3-d17460b5dd28) as an Engine plugin, or alternatively here via GitHub as a project-based plugin. Functionally, there are no differences.

Finally, [find and enable the plugin **ArticyXImporter** under **Edit > Plugins**](https://dev.epicgames.com/documentation/unreal-engine/working-with-plugins-in-unreal-engine). Restarting the editor may be required.

### <a name="project-based-plugin"></a>Project-based plugin

To install the plugin to your project you have basically two options:

- Download a release from GitHub and extract it to your project's `Plugins` directory
- Clone the Git repository to the project's `Plugins/ArticyXImporter` directory
  - Note that new clones point to the `main` branch by default. Checkout a tag to use a stable release of the plugin

At the end your project structure should look something like this:

<p align="center">
  <img src="https://www.articy.com/articy-importer/unreal/adx/copy_plugin.png">
</p>

### Engine-based plugin

If you decide to [get the plugin at Fab](https://www.fab.com/listings/e9edb068-c115-4fb0-a7f3-d17460b5dd28), the Epic Games Launcher will handle the installation for you.

> [!IMPORTANT]
> It is not possible to have both [ArticyXImporter for Unreal Engine](https://github.com/ArticySoftware/ArticyXImporterForUnreal) and [Articy 3 Importer for Unreal Engine](https://github.com/ArticySoftware/Articy3ImporterForUnreal) installed in the same engine or project. If you need to use both on the same engine installation, then you need to install the relevant plugin at the [project level](#project-based-plugin).

## How to continue

Check out the [project documentation](https://articysoftware.github.io/ArticyXImporterForUnreal/) to get started!

# Contributing

We are very grateful for any kind of contribution that you bring to the ArticyXImporter, no matter if it is reporting any issues, or by actively adding new features, or fixing existing issues. If you want to know more about how to contribute please check our [Contribution](CONTRIBUTING.md) article.

# Related resources

- [articy:draft X website](https://articy.com)
- [Project documentation](https://articysoftware.github.io/ArticyXImporterForUnreal/)
- [Maniac Manfred demo project](https://github.com/ArticySoftware/ArticyXDemoProjectForUnreal-ManiacManfred)
- Community
  - [Unofficial Discord server](https://discord.gg/M8PysxKKbJ)
  - [Articy Draft subreddit](https://www.reddit.com/r/articydraft/)
- Support
  - Contact us at [support@articy.com](mailto:support@articy.com)
