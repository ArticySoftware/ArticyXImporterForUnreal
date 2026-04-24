# Documentation Readme

This guide describes key aspects of how to write documentation for the *Articy X Importer Unreal plugin*. 

## Getting started

Basically, all you need to write documentation for this project is a text editor. All files that go into the documentation besides the source code are situated in the `docs` directory. `index.md` is the home page of the documentation. Further contents:

- `articles` directory contains articles about different aspects of *articy:draft* or the plugin and their interplay
- `css` contains some custom stylesheets
- `doxygen-awesome-css` is a submodule of the [Doxygen Awesome CSS repository](https://github.com/jothepro/doxygen-awesome-css). It provides some nicer styling as well as features like paragraph links and tabs.
- `images` contains images used in the articles
- `js` contains customised JavaScript, as the time of writing primarily an extended tabs feature (see below)
- `layout` contains customised layout file as per the [Doxygen Awesome CSS documentation](https://jothepro.github.io/doxygen-awesome-css/md_docs_2extensions.html)
- other images in this directory are referred to by the main readme file

## Writing documentation

The documentation is best opened in a text editor like VS Codium or an IDE of your choice. It should have support for [Markdown files](https://en.wikipedia.org/wiki/Markdown) at least. We're using [Doxygen](https://www.doxygen.nl) to generate the documentation website. It uses a modified version of Markdown so it supports it own markup language as well. The intricacies are fairly well documented on [Doxygen's documentation](https://www.doxygen.nl/manual/markdown.html).

In order to quickly check the correctness of the written documentation visually, it is advised to [install Doxygen](https://www.doxygen.nl/download.html) on your development machine. Make the Doxygen executable available on your `PATH`.

After applying changes to the documentation, use your favourite terminal to navigate to the project's directory, where the `Doxyfile` is located (root directory of the project). Run the Doxygen executable. After a few seconds, you will find a `docsBuild` directory in the project's directory. Open its `index.html` file to inspect the freshly built documentation.

### Maintaining the page tree

Pages may be organised in a tree structure. To do so, use Doxygen's `\page` and `\subpage` commands. For example, consider:

```doxygen
\page articles Articles

Find a list of articles here:

- \subpage basicObjectHandling
- \subpage buildServerIntegration
...
```
*Excerpt from `docs/articles/index.md`*

```doxygen
\page basicObjectHandling Basic Object Handling

When you export your data from **articy:draft X** into your Unreal project...
```
*Excerpt from `docs/articles/basic_object_handling.md`*

The `\page` line will be interpreted as the pages heading (e.g. "Articles") and will also be used in the menu. The lower-case "articles" or "basicObjectHandling" is an identifier that can be used with links or subpages. This can be observed in the `\subpage` commands that use the ID of the particular articles and creates a link to them. Also in the menu, those pages will be put under the current page's menu item, in this case "Articles".

### Authoring tabs

This project's documentation uses Doxygen Awesome CSS' tabs feature (in a customised way, see below) to allow the user to switch between C++ and Blueprint examples. Authoring such tabs is arguably inconvenient, but it serves the purpose.

In order to ease the [official syntax](https://jothepro.github.io/doxygen-awesome-css/md_docs_2extensions.html) we've introduced aliases using Doxygen's `ALIAS` feature. Consider the following example:

````doxygen
@tabs{
@tab{ C++ | cpp |
```cpp
UArticyObject* Manfred = UArticyDatabase::Get(WorldContext)->GetObjectByName("Chr_Manfred");
```
}
@tab{ Blueprint | uebp |
1) **Retrieve the Articy object:**
    - In the **Blueprint Editor**, you can use the node **Get Object By Name** on the **ArticyDatabase** type to retrieve an object by its technical name. Use **Cast To** to convert it to the appropriate type (e.g., `ManiacManfredCharacter`).

    ![Get Object By Name](bp_db_object_by_name.png)
}
}
````
*Contents have been trimmed for readability*

- `@tabs` begins a new tab section. Tabs have to be placed within its `{}`
- `@tab` begins a new tab. There are 3 parameters to pass within its `{}`:
    1. The label of the tab. This will be printed to the screen. (e.g. `C++`)
    2. (optional) The group identifier. This will be used to identify tabs of the same group for the code example language selection (see customisations below). (e.g. `cpp`)
    3. The contents

> [!note]
> Note that Markdown will handle content indented by 4 or more spaces as code blocks. It is advised to not indent tabs to evade this behaviour. When using lists this rule extends to 8 or more spaces, because lists allow to continue a list item's content by intenting by 4 spaces.

## Customisations

There are a few little customisations that we've built for the documentation. Most of them are visual, e.g. changed icons or colours. Those changes are primarily reflected in the stylesheets situated in the `docs/css` directory.

### Extended tabs

`docs/js/doxygen-awesome-tabs-customized.js` is an extended version of Doxygen Awesome CSS' tabs feature. It enables setting a *group* on a tab, which will be saved in the user browser's local storage. The last chosen group will be activated by default when loading a page of the documentation. This way we allow the user to switch between C++ and Blueprint examples tailored to their needs.

Also when selecting a tab that is assigned to a group, it will trigger all other tabs on the same page that have the same group.

## Deployment

Changes to the documentation are deployed according to the action defined in `.github/workflows/generate-docs.yml`.
