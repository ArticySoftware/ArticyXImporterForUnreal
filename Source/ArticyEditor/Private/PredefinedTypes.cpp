//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "PredefinedTypes.h"
#include "ArticyImportData.h"
#include "ArticyObject.h"
#include "ArticyBuiltinTypes.h"
#include "ArticyScriptFragment.h"

#define STRINGIFY(x) #x

//---------------------------------------------------------------------------//

// Ensure the static Map is filled with predefined types
FArticyPredefTypes FArticyPredefTypes::StaticInstance = FArticyPredefTypes{};

FArticyPredefTypes::FArticyPredefTypes()
{
	// Generic type for enums
	Enum = MakeShareable(PREDEFINE_TYPE_EXT(uint8, "", [](PROP_SETTER_PARAMS)
		{
			uint32 num;
			Json->TryGetNumber(num);
			return static_cast<uint8>(num);
		}));

	Types.Reset();

	// Add all predefined types
	// The key is the original type. It is NOT case-sensitive (FName)!
	// Some are exposed as values, some as pointers
	// Some can be initialized from JSON, others can't!

	Types.Add(TEXT("ArticyPrimitive"), new ArticyPredefinedTypeInfo<UArticyPrimitive, UArticyPrimitive*>("UArticyPrimitive", "UArticyPrimitive*", TEXT("nullptr"), nullptr /* NOTE: NO INITIALIZATION FROM JSON! */));
	Types.Add(TEXT("ArticyObject"), new ArticyPredefinedTypeInfo<UArticyObject, UArticyObject*>("UArticyObject", "UArticyObject*", TEXT("nullptr"), nullptr /* NOTE: NO INITIALIZATION FROM JSON! */));

	Types.Add(TEXT("id"), PREDEFINE_TYPE(FArticyId));
	Types.Add(TEXT("string"), PREDEFINE_TYPE_EXT(FString, "TEXT(\"\")", [](PROP_SETTER_PARAMS) { return Json->Type == EJson::String ? Json->AsString() : FString{}; }));

	const auto TextType = PREDEFINE_TYPE_EXT(FText, TEXT("FText::GetEmpty()"), [](PROP_SETTER_PARAMS)
		{
			if (Json->Type == EJson::String)
			{
				// Convert Unity rich text markup to Unreal (if the setting is enabled)
				const FString Processed = GetDefault<UArticyPluginSettings>()->bConvertUnityToUnrealRichText ?
					ConvertUnityMarkupToUnreal(Json->AsString()) :
					Json->AsString();

				// Return a new FText, where the Path is the key and the Property value is the default-language text
				return FText::ChangeKey(PackageName, Path, FText::FromString(Processed));
			}
			return FText::GetEmpty();
		});

	Types.Add(TEXT("ftext"), TextType);
	Types.Add(TEXT("rect"), PREDEFINE_TYPE(FArticyRect));
	Types.Add(TEXT("color"), PREDEFINE_TYPE_EXT(FLinearColor, "FLinearColor::Black", [](PROP_SETTER_PARAMS) { return ArticyHelpers::ParseColorFromJson(Json); }));
	Types.Add(TEXT("point"), PREDEFINE_TYPE_EXT(FVector2D, "FVector2D::ZeroVector", [](PROP_SETTER_PARAMS) { return ArticyHelpers::ParseFVector2DFromJson(Json); }));
	Types.Add(TEXT("size"), PREDEFINE_TYPE(FArticySize));
	Types.Add(TEXT("float"), PREDEFINE_TYPE_EXT(float, "0.f", [](PROP_SETTER_PARAMS) { return Json->IsNull() ? 0.f : static_cast<float>(Json->AsNumber()); }));
	Types.Add(TEXT("ArticyString"), TextType);
	Types.Add(TEXT("ArticyMultiLanguageString"), TextType);

	auto int32Info = PREDEFINE_TYPE_EXT(int32, "0", [](PROP_SETTER_PARAMS) {
		int32 num;
		Json->TryGetNumber(num);
		return num;
		});
	// uint is imported as int32 too, so we can expose it to blueprints
	Types.Add(TEXT("uint"), int32Info);
	Types.Add(TEXT("int"), int32Info);

	auto boolInfo = PREDEFINE_TYPE_EXT(bool, "false", [](PROP_SETTER_PARAMS) { return !Json->IsNull() && Json->AsBool(); });
	// bool and boolean are the same
	Types.Add(TEXT("bool"), boolInfo);
	Types.Add(TEXT("boolean"), boolInfo);

	Types.Add(TEXT("DateTime"), PREDEFINE_TYPE_EXT(FDateTime, "", [](PROP_SETTER_PARAMS)
		{
			FDateTime dt;
			if (Json->Type == EJson::String)
			{
				auto str = Json->AsString();
				int32 lastDot;
				if (str.FindLastChar('.', lastDot))
				{
					// Unreal only allows for up to 3 digits for the fractional second -.-
					auto pos = lastDot + 3;
					str.RemoveAt(pos, str.Len() - pos);
				}

				ensure(FDateTime::ParseIso8601(*str, dt));
			}
			return dt;
		}));

	// Generic arrays - the ItemType is filled in by FArticyPropertyDef
	// NOTE we cannot resolve the ItemType here, so we add a placeholder
	Types.Add(TEXT("array"), new ArticyPredefinedTypeInfo<nullptr_t>(TEXT("TArray<?>"), TEXT("TArray<?>"), "", nullptr));

	//========================================//

	// Preview image
	/*
	deprecated, we have generic enum solution
	Types.Add(TEXT("PreviewImageViewBoxModes"),PREDEFINE_TYPE_EXT(EArticyPreviewImageViewBoxModes, [](PROP_SETTER_PARAMS)
												{
													uint32 num;
													Json->TryGetNumber(num);
													return static_cast<EArticyPreviewImageViewBoxModes>(num);
												}));*/

	Types.Add(TEXT("PreviewImageViewBoxModes"), Enum.Get());

	Types.Add(TEXT("PreviewImage"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyPreviewImage));

	//========================================//

	// Connections
	Types.Add(TEXT("IncomingConnection"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyIncomingConnection));
	Types.Add(TEXT("OutgoingConnection"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyOutgoingConnection));

	//========================================//

	// Pins
	Types.Add(TEXT("InputPin"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyInputPin));
	Types.Add(TEXT("OutputPin"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyOutputPin));

	//========================================//

	// Script fragments
	Types.Add(TEXT("Script_Condition"), new ArticyObjectTypeInfo<UArticyScriptCondition, UArticyScriptCondition*>("UArticyScriptCondition", "UArticyScriptCondition""*"));
	Types.Add(TEXT("Script_Instruction"), new ArticyObjectTypeInfo<UArticyScriptInstruction, UArticyScriptInstruction*>("UArticyScriptInstruction", "UArticyScriptInstruction""*"));

	//========================================//

	// Locations
	/*
	deprecated, we have generic enum solution
	Types.Add(TEXT("LocationAnchorSize"),PREDEFINE_TYPE(EArticyLocationAnchorSize, [](PROP_SETTER_PARAMS)
											{
												uint32 num;
												Json->TryGetNumber(num);
												return static_cast<EArticyLocationAnchorSize>(num);
											}));*/
	Types.Add(TEXT("LocationAnchorSize"), Enum.Get());

	Types.Add(TEXT("LocationAnchor"), PREDEFINE_TYPE(FArticyLocationAnchor));

	Types.Add(TEXT("Transformation"), PREDEFINE_ARTICYOBJECT_TYPE(UArticyTransformation));
}

/**
 * Checks if a given type is a predefined type.
 *
 * @param OriginalType The name of the type to check.
 * @return True if the type is predefined, false otherwise.
 */
bool FArticyPredefTypes::IsPredefinedType(const FName& OriginalType)
{
	return StaticInstance.Types.Contains(OriginalType);
}

// Stores open tags in ConvertUnityMarkupToUnreal
struct TagInfo
{
	TagInfo(const FString& name, const FString& val)
		: tagName(name), hasValue(val.Len() > 0), value(val), dummy(false) {
		if (tagName == TEXT("align")) { dummy = true; }
	}

	// Tag name, like b, i, u, or color
	FString tagName;

	// Does this have a value? (like color="#FFFFFF")
	bool hasValue;

	// Value (if hasValue is true)
	FString value;

	// Dummy. Ignore this in the output
	bool dummy;
};

/**
 * Checks if there are any non-dummy tags in the list.
 *
 * @param currentTags The list of current tags.
 * @return True if there are any non-dummy tags, false otherwise.
 */
bool HasAnyTags(const TArray<TagInfo>& currentTags)
{
	for (const auto& tag : currentTags)
	{
		if (!tag.dummy) { return true; }
	}

	return false;
}

/**
 * Creates an open tag string from a list of current tags.
 *
 * @param currentTags The list of current tags.
 * @return A string representing the open tag.
 */
FString CreateOpenTag(const TArray<TagInfo>& currentTags)
{
	TArray<FString> tags;
	FString valueString = "";

	int numberOfTags = 0;
	for (const auto& tag : currentTags)
	{
		// Ignore dummy tags
		if (tag.dummy) continue;

		// If it's a value, append to the value string
		if (tag.hasValue) {
			valueString = valueString + FString::Printf(TEXT(" %s=\"%s\""), *tag.tagName, *tag.value);
		}
		else {
			// Otherwise, add the tag to the list
			tags.Add(tag.tagName);
		}
		numberOfTags++;
	}

	if (numberOfTags == 0) { return TEXT(""); }

	// Sort tag names
	tags.Sort([](const FString& a, const FString& b) { return (*a)[0] < (*b)[0]; });
	FString totalTags = "";
	for (const auto& tag : tags) { totalTags += tag; }

	// Handle no tags case
	if (totalTags.Len() == 0) { totalTags = "style"; }

	// Create tag
	return FString::Printf(TEXT("<%s%s>"), *totalTags, *valueString);
}

/**
 * Converts Unity markup to Unreal markup by transforming tag syntax.
 *
 * @param Input The input string with Unity markup.
 * @return The converted string with Unreal markup.
 */
FString ConvertUnityMarkupToUnreal(const FString& Input)
{
	// Regex pattern to match tags (start and end)
	static FRegexPattern Pattern(TEXT("<\\/(.+?)>|<(\\w+)([^>]*?)>"));

	// Create a matcher to search the input
	FRegexMatcher myMatcher(Pattern, Input);

	// Check to see if there's any matches at all
	bool anyMatches = myMatcher.FindNext();

	// If not, just return the input string
	if (!anyMatches) { return Input; }

	// Create a buffer to hold the output
	FString strings = "";

	// Run through matches
	TArray<TagInfo> currentTags;
	int last = 0;
	do
	{
		// Get bounds of match
		int start = myMatcher.GetMatchBeginning();
		int end = myMatcher.GetMatchEnding();

		// Add all text preceding the match to the output
		strings += (Input.Mid(last, start - last));

		// Check if we're dealing with a start tag or an end tag
		FString tagName = myMatcher.GetCaptureGroup(1);
		if (tagName.Len() > 0) {  // Handle start tags
			// Add to our list of open tags
			FString value = myMatcher.GetCaptureGroup(3);
			TagInfo info = TagInfo(tagName, value);
			currentTags.Add(info);

			// If it's not a dummy tag, open it in Unreal format
			if (!info.dummy)
			{
				strings += CreateOpenTag(currentTags);
			}
		}
		else {  // Handle closing tags
			if (currentTags.Num() == 0)
			{
				// Syntax issue, revert to original
				return Input;
			}

			// Remove the last opened tag
			auto popped = currentTags.Pop();

			// If it's not a dummy tag, close it in Unreal format
			if (!popped.dummy)
			{
				strings += TEXT("</>");
			}

			// Reopen any remaining tags in the correct order
			if (currentTags.Num() > 0)
			{
				strings += CreateOpenTag(currentTags);
			}
		}

		last = end;

	} while (myMatcher.FindNext());

	// Add any remaining text after the last match
	if (last != Input.Len())
	{
		strings += Input.Mid(last, Input.Len() - last);
	}

	// Create the final result string
	const FString result = DecodeHtmlEntities(strings);

	// Return the final result
	return result;
}

FString DecodeHtmlEntities(const FString& Input)
{
    FString DecodedString = Input;

    // 1. Replace standard named HTML entities
	static const TMap<FString, FString> HtmlEntities = {
		// Basic entities
		{TEXT("&lt;"), TEXT("<")},
		{TEXT("&gt;"), TEXT(">")},
		{TEXT("&amp;"), TEXT("&")},
		{TEXT("&quot;"), TEXT("\"")},
		{TEXT("&apos;"), TEXT("'")},
		{TEXT("&nbsp;"), TEXT("\u00A0")}, // Non-breaking space

		// Currency symbols
		{TEXT("&cent;"), TEXT("\u00A2")}, // ¢
		{TEXT("&pound;"), TEXT("\u00A3")}, // £
		{TEXT("&yen;"), TEXT("\u00A5")}, // ¥
		{TEXT("&euro;"), TEXT("\u20AC")}, // €
		{TEXT("&copy;"), TEXT("\u00A9")}, // ©
		{TEXT("&reg;"), TEXT("\u00AE")}, // ®

		// Math symbols
		{TEXT("&times;"), TEXT("\u00D7")}, // ×
		{TEXT("&divide;"), TEXT("\u00F7")}, // ÷
		{TEXT("&plusmn;"), TEXT("\u00B1")}, // ±
		{TEXT("&le;"), TEXT("\u2264")}, // ≤
		{TEXT("&ge;"), TEXT("\u2265")}, // ≥
	    {TEXT("&ne;"), TEXT("\u2260")}, // ≠
	    {TEXT("&infin;"), TEXT("\u221E")}, // ∞

	    // Greek letters
	    {TEXT("&alpha;"), TEXT("\u03B1")}, // α
	    {TEXT("&beta;"), TEXT("\u03B2")}, // β
	    {TEXT("&gamma;"), TEXT("\u03B3")}, // γ
	    {TEXT("&delta;"), TEXT("\u03B4")}, // δ
	    {TEXT("&epsilon;"), TEXT("\u03B5")}, // ε
	    {TEXT("&pi;"), TEXT("\u03C0")}, // π
	    {TEXT("&sigma;"), TEXT("\u03C3")}, // σ
	    {TEXT("&omega;"), TEXT("\u03C9")}, // ω

	    // Arrows
	    {TEXT("&larr;"), TEXT("\u2190")}, // ←
	    {TEXT("&uarr;"), TEXT("\u2191")}, // ↑
	    {TEXT("&rarr;"), TEXT("\u2192")}, // →
	    {TEXT("&darr;"), TEXT("\u2193")}, // ↓
	    {TEXT("&harr;"), TEXT("\u2194")}, // ↔
		{TEXT("&rArr;"), TEXT("\u21D2")}, // ⇒
		{TEXT("&lArr;"), TEXT("\u21D0")}, // ⇐

		// Punctuation
		{TEXT("&hellip;"), TEXT("\u2026")}, // …
		{TEXT("&middot;"), TEXT("\u00B7")}, // ·
		{TEXT("&laquo;"), TEXT("\u00AB")}, // «
		{TEXT("&raquo;"), TEXT("\u00BB")}, // »
		{TEXT("&ldquo;"), TEXT("\u201C")}, // “
		{TEXT("&rdquo;"), TEXT("\u201D")}, // ”
		{TEXT("&lsquo;"), TEXT("\u2018")}, // ‘
		{TEXT("&rsquo;"), TEXT("\u2019")}, // ’
		{TEXT("&ndash;"), TEXT("\u2013")}, // –
		{TEXT("&mdash;"), TEXT("\u2014")}, // —

		// Fractions
		{TEXT("&frac14;"), TEXT("\u00BC")}, // ¼
		{TEXT("&frac12;"), TEXT("\u00BD")}, // ½
		{TEXT("&frac34;"), TEXT("\u00BE")}, // ¾

		// Miscellaneous symbols
		{TEXT("&para;"), TEXT("\u00B6")}, // ¶
		{TEXT("&sect;"), TEXT("\u00A7")}, // §
		{TEXT("&dagger;"), TEXT("\u2020")}, // †
		{TEXT("&Dagger;"), TEXT("\u2021")}, // ‡
		{TEXT("&bull;"), TEXT("\u2022")}, // •
		{TEXT("&trade;"), TEXT("\u2122")}, // ™
		{TEXT("&spades;"), TEXT("\u2660")}, // ♠
		{TEXT("&clubs;"), TEXT("\u2663")}, // ♣
		{TEXT("&hearts;"), TEXT("\u2665")}, // ♥
		{TEXT("&diams;"), TEXT("\u2666")}, // ♦
	};
	
    for (const auto& EntityPair : HtmlEntities)
    {
        DecodedString = DecodedString.Replace(*EntityPair.Key, *EntityPair.Value);
    }

    // 2. Decode numeric character references (decimal and hex)
    static const FRegexPattern NumericEntityPattern(TEXT(R"(&#(x[0-9A-Fa-f]+|\d+);)"));

    FRegexMatcher Matcher(NumericEntityPattern, DecodedString);

    FString FinalString;
    int32 LastPosition = 0;

    while (Matcher.FindNext())
    {
        // Append text before the entity
        FinalString += DecodedString.Mid(LastPosition, Matcher.GetMatchBeginning() - LastPosition);

        // Extract entity value
        FString Entity = Matcher.GetCaptureGroup(1);
        int32 CodePoint = 0;

        if (Entity.StartsWith(TEXT("x")) || Entity.StartsWith(TEXT("X")))
        {
            // Hexadecimal numeric entity
            FString HexPart = Entity.Mid(1);
            CodePoint = FParse::HexNumber(*HexPart);
        }
        else
        {
            // Decimal numeric entity
            CodePoint = FCString::Atoi(*Entity);
        }

        // Convert Unicode code point to TCHAR
        if (CodePoint > 0)
        {
            if (CodePoint <= 0xFFFF) // Basic Multilingual Plane (BMP)
            {
                FinalString.AppendChar(static_cast<TCHAR>(CodePoint));
            }
            else
            {
                // Convert code point outside BMP to UTF-16 surrogate pair
                int32 HighSurrogate = ((CodePoint - 0x10000) >> 10) + 0xD800;
                int32 LowSurrogate = ((CodePoint - 0x10000) & 0x3FF) + 0xDC00;

                FinalString.AppendChar(static_cast<TCHAR>(HighSurrogate));
                FinalString.AppendChar(static_cast<TCHAR>(LowSurrogate));
            }
        }

        // Update last position
        LastPosition = Matcher.GetMatchEnding();
    }

    // Append remaining text after the last entity
    if (LastPosition < DecodedString.Len())
    {
        FinalString += DecodedString.Mid(LastPosition);
    }

    return FinalString;
}

