\page commonIssues Common Issues

This page covers common issues developers encounter when using the **ArticyX Importer** plugin for **Unreal Engine**. It also includes solutions based on feedback from support cases. Please refer to this page if you run into problems with importing, asset packaging, or localization.

By addressing these common issues, you should be able to avoid many of the pitfalls encountered when using the **ArticyX Importer** plugin for **Unreal Engine**.

## Error: Could not get Articy Database when Running a Packaged Build

**Issue**: The generated **Articy Database** and packages are not included in the final packaged build, causing errors when running the game.

**Solution**:
- Make sure the **ArticyContent** folder is listed under **Additional Asset Directories to Cook** in your project’s **Packaging settings**.

To verify that the **Articy** assets are making it into the packaged build, use the **UnrealPak** utility to extract the packaged content and check for the bundled assets:

```
UnrealPak.exe "C:\Path\To\Your\ShippingBuild\WindowsNoEditor\ProjectName\Content\Paks\ProjectName.pak" -Extract "C:\Path\To\Extract\To"
```

Replace the paths with the location of your build and an extraction directory. Check if the **Articy Generated** assets are present in the **ArticyContent/Generated** folder.

The **UnrealPak** utility is located in the `\Engine\Binaries` folder of your Unreal installation.

---

## Files Marked as Delete in Perforce/Plastic SCM

**Issue**: Older versions of the plugin had problems with source control, particularly with **Perforce** and **Plastic SCM**, where generated **Articy assets** would be marked as delete instead of edit.

**Solution**:
- Ensure you are using version **1.3.0** or higher of the plugin, which resolves this issue.
- If the problem persists, please contact **support\@articy.com** or create an issue on GitHub.

---

## Articy Assets (Textures/Sounds) Not Appearing in Unreal

**Issue**: Imported **Articy assets** such as textures and sounds are missing in **Unreal**.

**Solution**:
- Verify that you are exporting the project directly into your Unreal game directory. The **.articyue** file and **ArticyContent** folder should be present.
- Both the **.articyue** file and **ArticyContent** directory need to be copied into your **Unreal** project folder.

---

## Hot Reloading Drop-Down List Changes

**Issue**: Due to a known **Unreal** issue with hot reloading changes to enums, using hot reload on drop-down list changes can break the **Blueprint nodes**, converting them into bytes.

**Solution**:
- Avoid hot reloading when importing changes to drop-down lists.
- Always restart Unreal to avoid issues with drop-down list nodes in Blueprints.

---

## Error C2451: Invalid Conditional Expression in TWeakObjectPtr

**Issue**: After updating to version 1.4 of the **Articy Unreal** plugin, this error appears:

```
Error C2451: a conditional expression of type 'const TWeakObjectPtr<UObject,FWeakObjectPtr>' is not valid.
```

**Solution**:
- Manually edit the generated C++ file to resolve the issue. Locate the `GetUserMethodsProviderObject` method in `{ProjectName}ExpressoScripts.h` (in the **ArticyGenerated** folder) and delete it.

---

## String Table Localization Not Included in Packaged Build

**Issue**: **CSV files** used for string table localization are not included in the packaged build by default.

**Solution**:
- Add the **ArticyContent/Generated** directory to the **Additional Non-Asset Directories to Package** in your project’s **Packaging settings**.

---

## Localized Feature Templates Showing Localization IDs

**Issue**: Localized feature template variables show localization IDs instead of the actual localized text in Unreal.

**Solution**:
- Ensure that the **Multi-Language?** property is ticked in the feature settings in **Articy**.
- Verify that you're using the **Get<Name>(Localized)** method for accessing localized feature properties in **Blueprints**.
- Make sure that you are using the latest version of the **ArticyX Importer** plugin and have done a full reimport with the updated plugin.

---

## Packaging Errors with Missing Text

**Issue**: When packaging a game with the **ArticyX** plugin in **Unreal Engine 5**, text does not display in the final build, although the flow works correctly.

**Solution**:
- Ensure that the **ArticyContent** directory is added under **Additional Non-Asset Directories to Package** in the **Packaging settings**.
- Verify that the directory is not added under the **Cook** section by mistake.

---

## No "Start On" Options Appearing in a New Project

**Issue**: After following the tutorial, the **Start On** options do not appear in the flow player for a new project.

**Solution**:
- Make sure your project is a **C++ project** and not just a **Blueprint** project. If the project is missing a **Build.cs** file, it may be treated as a Blueprint project.
- Add any **C++ class** to the project to resolve the issue. If the problem persists, create a new project and migrate your assets.

---

## Further Assistance

If you encounter additional issues not covered in this document, please contact **Articy Support** at **support\@articy.com** for further assistance. Make sure your project is updated to the latest plugin version before reporting any bugs.
