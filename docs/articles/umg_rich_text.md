\page umgRichText UMG Rich Text support

If your articy:draft X project has been exported using either the Unity Rich Text or Extended Markup formatting settings, you can use articy with the Unreal Rich Text Block widget to display richly formatted text.

### Export and Import Configuration

First, make sure you select one of these two settings in the `Export options` dialog in articy:draft X

![](urt_export_settings.png)

Then, you'll need to enable a setting in Unreal to convert Unity rich text formatting to Unreal's format. **NOTE: Make sure you click `Import Changes` anytime you change this setting. You can find it in the [Articy Import Window](#import-into-unreal).**

![](urt_plugin_settings.png)

### Configuring Styles

Next, you have to configure your styles by creating a style data table in Unreal. This asset tells the rich text widget what fonts, colors, and other styling effects to apply to each kind of text. Create this asset by creating a new `Data Table` in your Content window (under `Miscellaneous`) and for the row structure, pick `RichTextStyleRow`.

![](urt_datatable_settings.png)

Next, open the new data table and create your styles. You'll need to import a few fonts into your project to do this. On Windows, you can find the fonts installed under `C:\Windows\Fonts`. Drag one into your Unreal project to import it.

You'll need to create a new row for every combination of styles you want to support. The first row is always the default style: what you'll get if there's no formatting set on text.

Create a `b` row for a bold style. You can also create an `i` row for italic and `u` for underline. Make sure to set the appropriate font and style setting for each type.

To support combinations (such as bold **and** italic), you need to create combination rows. Each combination is always alphabetically ordered. So, to support bold and italic, you'd create a `bi` row. For bold and underline, `bu`. For all three: `biu`. You need to create a row for each unique combination to tell Unreal which font to use. Usually, when importing a font, you'll get versions for each combination.

**Example:**

![](urt_datatable.png)

### Configuring your Rich Text Control

Once these are all set, you can configure your rich text control. Create a Rich Text block in the UI editor and set it's Text Style Set to your new data table.

![](urt_richtext_control.png)

Now, you'll be able to see your styling in articy appear in Unreal! Try setting the text to the text of a formatted node in articy to test.

### Color Support

*Note: Colors are only available with the extended markup option in the articy:draft X export.*

To support additional styling like custom colors, you need to add the `ArticyRichTextDecorator` to the list of `Decorator classes` on the rich text block.

![](urt_richtext_decorator.png)

### Hyperlinks

*Note: Hyperlinks are only available with the extended markup option in the articy:draft X export.*

To use hyperlinks from Articy, you'll need to do two things:

1. Add the interface `ArticyHyperlinkHandler` to your user widget that owns the rich text control. You can do this in `Class Settings` under interfaces. Then, implement the `On Hyperlink Navigated` method to catch the event of users clicking the hyperlinks.
2. Sub-class `ArticyRichTextDecorator` with a new Blueprint class and configure its `HyperlinkStyle` property. This will control the regular, hover, and underline style behavior of the hyperlinks. Then, use this new blueprint class as your Decorator class on the rich text control instead of `ArticyRichTextDecorator`.
