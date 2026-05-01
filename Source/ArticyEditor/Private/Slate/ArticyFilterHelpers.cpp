//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/ArticyFilterHelpers.h"
#include "ArticyObject.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithText.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "ArticyEditorModule.h"

#define LOCTEXT_NAMESPACE "ArticyObjectSearchBoxHelpers"

/**
 * Expression context to test the given asset data against the current text filter.
 * This class evaluates whether an Articy object matches the provided filter criteria.
 */
class FFrontendFilter_ArticyObjectFilterExpressionContext : public ITextFilterExpressionContext
{
public:
	// Define a pointer type for FArticyObjectFilterType without const and reference qualifiers.
	typedef TRemoveReference<FArticyObjectFilterType>::Type* FArticyObjectFilterTypePtr;

	/** Default constructor for FFrontendFilter_ArticyObjectFilterExpressionContext. */
	FFrontendFilter_ArticyObjectFilterExpressionContext()
		: AssetPtr(nullptr)
		, bIncludeClassName(true)
		, NameKeyName("Name")
		, PathKeyName("Path")
		, ClassKeyName("Class")
		, TypeKeyName("Type")
		, TagKeyName("Tag")
	{
	}

	/**
	 * Sets the asset to be filtered.
	 *
	 * @param InAsset Pointer to the asset being filtered.
	 */
	void SetAsset(FArticyObjectFilterTypePtr InAsset)
	{
		AssetPtr = InAsset;
	}

	/** Clears the current asset from the context. */
	void ClearAsset()
	{
		AssetPtr = nullptr;
	}

	/**
	 * Sets whether the class name should be included in the filter.
	 *
	 * @param InIncludeClassName Boolean value indicating inclusion of class name.
	 */
	void SetIncludeClassName(const bool InIncludeClassName)
	{
		bIncludeClassName = InIncludeClassName;
	}

	/**
	 * Gets whether the class name is included in the filter.
	 *
	 * @return Boolean value indicating inclusion of class name.
	 */
	bool GetIncludeClassName() const
	{
		return bIncludeClassName;
	}

	/**
	 * Tests the asset against a basic string expression.
	 *
	 * @param InValue The value to test against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the asset matches the string expression, false otherwise.
	 */
	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		// Cast the asset pointer to UArticyObject type.
		UArticyObject* ArticyObject = Cast<UArticyObject>(AssetPtr->GetAsset());

		// Check the asset's display name, technical name, text, and speaker display name.
		if (CheckDisplayName(InValue, InTextComparisonMode) ||
			CheckTechnicalName(InValue, InTextComparisonMode) ||
			CheckTextOfArticyObject(InValue, InTextComparisonMode) ||
			CheckSpeakerDisplayName(InValue, InTextComparisonMode))
		{
			return true;
		}

		// Compare asset name.
		const FTextFilterString AssetName(AssetPtr->AssetName);
		if (AssetName.CompareText(InValue, InTextComparisonMode))
		{
			return true;
		}

		// Optionally include class name in the filter.
		if (bIncludeClassName)
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
			const FTextFilterString AssetClassFilter(AssetPtr->AssetClassPath.ToString());
#else
			const FTextFilterString AssetClassFilter(AssetPtr->AssetClass);
#endif
			if (AssetClassFilter.CompareText(InValue, InTextComparisonMode))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Tests the asset against a complex expression.
	 *
	 * @param InKey The key for the expression.
	 * @param InValue The value to test against.
	 * @param InComparisonOperation The comparison operation to use.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the asset matches the complex expression, false otherwise.
	 */
	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		// Special handling for the asset name, path, and type keys.
		if (InKey == NameKeyName)
		{
			// Only Equal or NotEqual operations are allowed.
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			// Compare the asset name against the filter value.
			const bool bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->AssetName, InValue, InTextComparisonMode);
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		if (InKey == PathKeyName)
		{
			// Only Equal or NotEqual operations are allowed.
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			// Compare the asset path in either Partial or Exact mode.
			bool bIsMatch = false;
			if (InTextComparisonMode == ETextFilterTextComparisonMode::Partial)
			{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->GetObjectPathString(), InValue, InTextComparisonMode);
#else
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->ObjectPath, InValue, InTextComparisonMode);
#endif
			}
			else
			{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->GetObjectPathString(), InValue, InTextComparisonMode)
#else
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->ObjectPath, InValue, InTextComparisonMode)
#endif
					|| TextFilterUtils::TestBasicStringExpression(AssetPtr->PackageName, InValue, InTextComparisonMode)
					|| TextFilterUtils::TestBasicStringExpression(AssetPtr->PackagePath, InValue, InTextComparisonMode);
			}
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		if (InKey == ClassKeyName || InKey == TypeKeyName)
		{
			// Only Equal or NotEqual operations are allowed.
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
			const bool bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->AssetClassPath.ToString(), InValue, InTextComparisonMode);
#else
			const bool bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->AssetClass, InValue, InTextComparisonMode);
#endif	
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		// Generic handling for anything in the asset meta-data
		{
			auto GetMetaDataValue = [this, &InKey](FString& OutMetaDataValue) -> bool
				{
					// Check for a literal key
					if (AssetPtr->GetTagValue(InKey, OutMetaDataValue))
					{
						return true;
					}

					return false;
				};

			FString MetaDataValue;
			if (GetMetaDataValue(MetaDataValue))
			{
				return TextFilterUtils::TestComplexExpression(MetaDataValue, InValue, InComparisonOperation, InTextComparisonMode);
			}
		}

		return false;
	}

private:

	/** Pointer to the asset currently being filtered */
	FArticyObjectFilterTypePtr AssetPtr;

	/** Boolean to indicate if the class name should be included in string tests */
	bool bIncludeClassName;

	/** Keys used for complex expression tests */
	const FName NameKeyName;
	const FName PathKeyName;
	const FName ClassKeyName;
	const FName TypeKeyName;
	const FName TagKeyName;

	/**
	 * Checks if the display name of the asset matches the filter value.
	 *
	 * @param InValue The filter value to compare against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the display name matches, false otherwise.
	 */
	bool CheckDisplayName(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
	{
		// Check if the asset implements IArticyObjectWithDisplayName
		IArticyObjectWithDisplayName* ArticyObjectWithDisplayName = Cast<IArticyObjectWithDisplayName>(AssetPtr->GetAsset());

		if (ArticyObjectWithDisplayName)
		{
			// Get the display name and compare it with the filter value.
			const FTextFilterString TextToCompare(ArticyObjectWithDisplayName->GetDisplayName().ToString());

			if (TextToCompare.IsEmpty())
			{
				return false;
			}

			return TextToCompare.CompareText(InValue, InTextComparisonMode);
		}

		return false;
	}

	/**
	 * Checks if the text of the Articy object matches the filter value.
	 *
	 * @param InValue The filter value to compare against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the text matches, false otherwise.
	 */
	bool CheckTextOfArticyObject(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
	{
		// Check if the asset implements IArticyObjectWithText
		IArticyObjectWithText* ArticyObjectWithText = Cast<IArticyObjectWithText>(AssetPtr->GetAsset());

		if (ArticyObjectWithText)
		{
			// Get the text and compare it with the filter value.
			const FText Text = ArticyObjectWithText->GetText();
			const FTextFilterString FilterString(Text.ToString());

			const bool bTextIsEmptyOrWhitespace = Text.IsEmptyOrWhitespace();

			if (!bTextIsEmptyOrWhitespace && FilterString.CompareText(InValue, InTextComparisonMode))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Checks if the technical name of the Articy object matches the filter value.
	 *
	 * @param InValue The filter value to compare against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the technical name matches, false otherwise.
	 */
	bool CheckTechnicalName(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
	{
		// Cast the asset pointer to UArticyObject
		UArticyObject* ArticyObject = Cast<UArticyObject>(AssetPtr->GetAsset());

		if (ArticyObject)
		{
			// Get the technical name and compare it with the filter value.
			const FTextFilterString TextToCompare(ArticyObject->GetTechnicalName());

			if (TextToCompare.IsEmpty())
			{
				return false;
			}

			return TextToCompare.CompareText(InValue, InTextComparisonMode);
		}

		return false;
	}

	/**
	 * Checks if the speaker display name of the Articy object matches the filter value.
	 *
	 * @param InValue The filter value to compare against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the speaker display name matches, false otherwise.
	 */
	bool CheckSpeakerDisplayName(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
	{
		// Check if the asset implements IArticyObjectWithSpeaker
		IArticyObjectWithSpeaker* ArticyObjectWithSpeaker = Cast<IArticyObjectWithSpeaker>(AssetPtr->GetAsset());

		if (ArticyObjectWithSpeaker)
		{
			// Find the speaker asset and check its display name.
			IArticyObjectWithDisplayName* SpeakerDisplayName = Cast<IArticyObjectWithDisplayName>(UArticyObject::FindAsset(ArticyObjectWithSpeaker->GetSpeakerId()));

			if (!SpeakerDisplayName)
			{
				UE_LOG(LogArticyEditor, Error, TEXT("Articy filter: Speaker object does not exist"));
				return false;
			}

			// Compare the speaker's display name with the filter value.
			const FText& SpeakerName = SpeakerDisplayName->GetDisplayName();
			const FTextFilterString TextToCompare(SpeakerName.ToString());

			if (TextToCompare.IsEmpty())
			{
				return false;
			}

			return TextToCompare.CompareText(InValue, InTextComparisonMode);
		}

		return false;
	}
};

/** Constructor for FFrontendFilter_ArticyObject. */
FFrontendFilter_ArticyObject::FFrontendFilter_ArticyObject() :
	TextFilterExpressionContext(MakeShareable(new FFrontendFilter_ArticyObjectFilterExpressionContext()))
	, TextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::Complex)
{
}

/** Destructor for FFrontendFilter_ArticyObject. */
FFrontendFilter_ArticyObject::~FFrontendFilter_ArticyObject()
{

}

/**
 * Determines if the given item passes the filter.
 *
 * @param InItem The item to test against the filter.
 * @return True if the item passes the filter, false otherwise.
 */
bool FFrontendFilter_ArticyObject::PassesFilter(FArticyObjectFilterType InItem) const
{
	// Set the asset to be filtered and evaluate the filter expression.
	TextFilterExpressionContext->SetAsset(&InItem);
	const bool bMatched = TextFilterExpressionEvaluator.TestTextFilter(*TextFilterExpressionContext);
	TextFilterExpressionContext->ClearAsset();
	return bMatched;
}

/**
 * Retrieves the raw filter text.
 *
 * @return The raw filter text.
 */
FText FFrontendFilter_ArticyObject::GetRawFilterText() const
{
	return TextFilterExpressionEvaluator.GetFilterText();
}

/**
 * Sets the raw filter text.
 *
 * @param InFilterText The filter text to set.
 */
void FFrontendFilter_ArticyObject::SetRawFilterText(const FText& InFilterText)
{
	if (TextFilterExpressionEvaluator.SetFilterText(InFilterText))
	{
		// Will trigger a re-filter with the new text
		BroadcastChangedEvent();
	}
}

/**
 * Retrieves any error text from the filter.
 *
 * @return The filter error text.
 */
FText FFrontendFilter_ArticyObject::GetFilterErrorText() const
{
	return TextFilterExpressionEvaluator.GetFilterErrorText();
}

/**
 * Sets whether the class name should be included in the filter.
 *
 * @param InIncludeClassName Boolean value indicating inclusion of class name.
 */
void FFrontendFilter_ArticyObject::SetIncludeClassName(const bool InIncludeClassName)
{
	if (TextFilterExpressionContext->GetIncludeClassName() != InIncludeClassName)
	{
		TextFilterExpressionContext->SetIncludeClassName(InIncludeClassName);

		// Will trigger a re-filter with the new setting
		BroadcastChangedEvent();
	}
}

/**
 * Constructor for FArticyClassRestrictionFilter.
 *
 * @param InAllowedClass The class to be allowed by the filter.
 * @param bInExactClass Whether the class restriction is exact.
 */
FArticyClassRestrictionFilter::FArticyClassRestrictionFilter(TSubclassOf<UArticyObject> InAllowedClass, bool bInExactClass) : AllowedClass(InAllowedClass), bExactClass(bInExactClass)
{
}

/**
 * Determines if the given item passes the class restriction filter.
 *
 * @param InItem The item to test against the filter.
 * @return True if the item passes the filter, false otherwise.
 */
bool FArticyClassRestrictionFilter::PassesFilter(FArticyObjectFilterType InItem) const
{
	// Check if the asset matches the allowed class type.
	if (bExactClass)
	{
		return InItem.GetClass() == AllowedClass;
	}

	return InItem.GetAsset()->IsA(AllowedClass.Get());
}

/** Expression context to test the given variable data against the current text filter */
class FFrontendFilter_ArticyGVFilterExpressionContext : public ITextFilterExpressionContext
{
public:
	typedef TRemoveReference<FArticyVariableFilterType>::Type FArticyVariableFilterTypePtr;

	/** Default constructor for FFrontendFilter_ArticyGVFilterExpressionContext. */
	FFrontendFilter_ArticyGVFilterExpressionContext()
		: VariablePtr(nullptr)
	{}

	/**
	 * Sets the variable to be filtered.
	 *
	 * @param InVariable Pointer to the variable being filtered.
	 */
	void SetVariable(FArticyVariableFilterTypePtr InVariable)
	{
		VariablePtr = InVariable;
	}

	/** Clears the current variable from the context. */
	void ClearVariable()
	{
		VariablePtr = nullptr;
	}

	/**
	 * Tests the variable against a basic string expression.
	 *
	 * @param InValue The value to test against.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the variable matches the string expression, false otherwise.
	 */
	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const override;

	/**
	 * Tests the variable against a complex expression.
	 *
	 * @param InKey The key for the expression.
	 * @param InValue The value to test against.
	 * @param InComparisonOperation The comparison operation to use.
	 * @param InTextComparisonMode The mode of text comparison.
	 * @return True if the variable matches the complex expression, false otherwise.
	 */
	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue,
		const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const override;

private:

	FArticyVariableFilterTypePtr VariablePtr;
};

/**
 * Tests the variable against a basic string expression.
 *
 * @param InValue The value to test against.
 * @param InTextComparisonMode The mode of text comparison.
 * @return True if the variable matches the string expression, false otherwise.
 */
bool FFrontendFilter_ArticyGVFilterExpressionContext::TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
{
	// Retrieve the variable name and set name, and compare them against the filter value.
	const UArticyVariable* Variable = VariablePtr;

	const FTextFilterString VarNameToCompare(Variable->GetName());
	if (VarNameToCompare.CompareText(InValue, InTextComparisonMode))
	{
		return true;
	}

	const FTextFilterString VarSetNameToCompare = Variable->GetOuter()->GetName();
	if (VarSetNameToCompare.CompareText(InValue, InTextComparisonMode))
	{
		return true;
	}

	return false;
}

/**
 * Tests the variable against a complex expression.
 *
 * @param InKey The key for the expression.
 * @param InValue The value to test against.
 * @param InComparisonOperation The comparison operation to use.
 * @param InTextComparisonMode The mode of text comparison.
 * @return False, as complex expressions are not used for variables.
 */
bool FFrontendFilter_ArticyGVFilterExpressionContext::TestComplexExpression(const FName& InKey, const FTextFilterString& InValue,
	const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const
{
	// Don't use this filter
	return false;
}

/** Constructor for FFrontendFilter_ArticyVariable. */
FFrontendFilter_ArticyVariable::FFrontendFilter_ArticyVariable() :
	TextFilterExpressionContext(MakeShareable(new FFrontendFilter_ArticyGVFilterExpressionContext())),
	TextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString)
{
}

/** Destructor for FFrontendFilter_ArticyVariable. */
FFrontendFilter_ArticyVariable::~FFrontendFilter_ArticyVariable()
{
}

/**
 * Determines if the given variable passes the filter.
 *
 * @param InItem The variable to test against the filter.
 * @return True if the variable passes the filter, false otherwise.
 */
bool FFrontendFilter_ArticyVariable::PassesFilter(FArticyVariableFilterType InItem) const
{
	// Set the variable to be filtered and evaluate the filter expression.
	TextFilterExpressionContext->SetVariable(InItem);
	const bool bMatched = TextFilterExpressionEvaluator.TestTextFilter(*TextFilterExpressionContext);
	TextFilterExpressionContext->ClearVariable();
	return bMatched;
}

/**
 * Retrieves the raw filter text.
 *
 * @return The raw filter text.
 */
FText FFrontendFilter_ArticyVariable::GetRawFilterText() const
{
	return TextFilterExpressionEvaluator.GetFilterText();
}

/**
 * Sets the raw filter text.
 *
 * @param InFilterText The filter text to set.
 */
void FFrontendFilter_ArticyVariable::SetRawFilterText(const FText& InFilterText)
{
	if (TextFilterExpressionEvaluator.SetFilterText(InFilterText))
	{
		// Will trigger a re-filter with the new text
		ChangedEvent.Broadcast();
	}
}

/**
 * Retrieves any error text from the filter.
 *
 * @return The filter error text.
 */
FText FFrontendFilter_ArticyVariable::GetFilterErrorText() const
{
	return TextFilterExpressionEvaluator.GetFilterErrorText();
}

#undef LOCTEXT_NAMESPACE
