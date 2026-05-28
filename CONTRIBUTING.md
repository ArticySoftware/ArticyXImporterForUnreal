# Contribute

Thank you very much for considering to help us in developing the Articy X Importer for Unreal plugin. 
All help is appreciated, from giving feedback and reporting bugs to actively contributing new features and releasing them for all users. 

## Giving feedback and getting help

[The unofficial community Discord](https://discord.gg/bVATb5evhZ) or the [articy:draft subreddit](https://www.reddit.com/r/articydraft/) are good way to receive quick help and post feedback. If you have a 
problem related to the articy:draft software please contact our support at [support@articy.com](mailto:support@articy.com).

## Reporting issues

If you found a bug or a problem:

 * Make sure the bug is not already posted by searching [all the issues](https://github.com/ArticySoftware/ArticyXImporterForUnreal/issues?q=).
 * If no entry was found you should [open a new issue](https://github.com/ArticySoftware/ArticyXImporterForUnreal/issues/new). Please include a title and try to provide as much information as possible. Providing a stripped-down example in a new project with as little steps as possible to reproduce the issue increases the chances of fixing an issue tremendously.

## Actively changing code

Implementing features and fixing bugs take time to develop and the best way to help us is by stepping in and providing those features and fixes yourself. 
To do that you just need to get the sources from here on GitHub, your best bet is to [fork the repository](https://help.github.com/articles/fork-a-repo/). When you are happy with your changes you should send a [pull request](https://help.github.com/articles/creating-a-pull-request/) so we can review and test your changes.

**Before submitting a contribution:**

- Read the documentation carefully and find out if the functionality is already covered, maybe by an individual configuration
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset
- Make sure to base your contribution off the updated `main` branch

### Branching strategy

Changes for the upcoming version are integrated in the `main` branch. `main` always reflects the current state of development, so it might be unstable at times. To get a stable version of the plugin, refer to the latest release. Features or bugfixes that cannot be finished for an upcoming released are developed in separate branches. Support for older versions, if necessary, is done in separate release branches:

- `main`: latest development version, possibly unstable
- `feature/x` or `bugfix/x`: comprehensive developments that could block releases on `main` are developed on separate branches
- `release/1.5.x`: if support for older versions becomes mandatory, they will be carried out on separate release branches

### Pull Requests workflow

- Fork this repository and apply your changes on any branch
- For usual changes, create a pull request from your fork's branch to this repository's `main` branch
- For comprehensive developments, a feature or bugfix branch may be created by the maintainers
- Any PR towards this repository must be reviewed and approved by at least 1 maintainer
- Use [conventional commits](https://www.conventionalcommits.org/en/v1.0.0/) for everything that is pushed on `main`
  - If necessary, pull requests will be squashed and merged onto `main` for a clean history
- Maintainers may work on this repository directly

### Tags and releases

Tags and releases are created by the project's maintainers. Only releases without a suffix are considered stable.
