//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyType.h"

#include "ArticyTypeSystem.h"

FArticyEnumValueInfo FArticyType::GetEnumValue(int Value) const
{
	for (const auto EnumInfo : EnumValues)
	{
		if (EnumInfo.Value == Value)
		{
			return EnumInfo;
		}
	}
	return {};
}

FArticyEnumValueInfo FArticyType::GetEnumValue(const FString& ValueName) const
{
	for (const auto EnumInfo : EnumValues)
	{
		if (EnumInfo.LocaKey_DisplayName.Equals(ValueName))
		{
			return EnumInfo;
		}
	}
	return {};
}

FString FArticyType::GetFeatureDisplayName(const FString& FeatureName) const
{
	return LocalizeString(FeatureName);
}

FString FArticyType::GetFeatureDisplayNameLocaKey(const FString& FeatureName) const
{
	return FeatureName;
}

TArray<FArticyPropertyInfo> FArticyType::GetProperties() const
{
	return Properties;
}

TArray<FArticyPropertyInfo> FArticyType::GetPropertiesInFeature(const FString& FeatureName) const
{
	// Return most precise match
	FArticyType FeatureType = UArticyTypeSystem::Get()->GetArticyType(TechnicalName + "." + FeatureName);
	if (FeatureType.IsInvalidType)
	{
		FeatureType = UArticyTypeSystem::Get()->GetArticyType(LocaKey_DisplayName + "." + FeatureName);
	}
	if (FeatureType.IsInvalidType)
	{
		FeatureType = UArticyTypeSystem::Get()->GetArticyType(FeatureName);
	}
	return FeatureType.Properties;
}

FArticyPropertyInfo FArticyType::GetProperty(const FString& PropertyName) const
{
	// Find most precise match for property
	
	for (const auto PropertyInfo : Properties)
	{
		if (PropertyInfo.TechnicalName.Equals(PropertyName))
		{
			return PropertyInfo;
		}
	}

	for (const auto PropertyInfo : Properties)
	{
		if (PropertyInfo.LocaKey_DisplayName.Equals(PropertyName))
		{
			return PropertyInfo;
		}
	}

	// Invalid property placeholder
	FArticyPropertyInfo InvalidProperty;
	InvalidProperty.IsInvalidProperty = true;
	return InvalidProperty;
}

FString FArticyType::LocalizeString(const FString& Input)
{
	const FText MissingEntry = FText::FromString("<MISSING STRING TABLE ENTRY>");

	// Look up entry in specified string table
	TOptional<FString> TableName = FTextInspector::GetNamespace(FText::FromString(Input));
	if (!TableName.IsSet())
	{
		TableName = TEXT("ARTICY");
	}
	const FText SourceString = FText::FromStringTable(
		FName(TableName.GetValue()),
		Input,
		EStringTableLoadingPolicy::FindOrFullyLoad);
	const FString Decoded = SourceString.ToString();
	if (!SourceString.IsEmpty() && !SourceString.EqualTo(MissingEntry))
	{
		return SourceString.ToString();
	}

	// By default, return input
	return Input;
}

void FArticyType::MergeChild(const FArticyType& Child)
{
	HasTemplate |= Child.HasTemplate;
	IsEnum |= Child.IsEnum;
	if (!Child.CPPType.IsEmpty())
	{
		CPPType = Child.CPPType;
	}
	if (!Child.DisplayName.IsEmpty())
	{
		DisplayName = Child.DisplayName;
	}
	if (!Child.LocaKey_DisplayName.IsEmpty())
	{
		LocaKey_DisplayName = Child.LocaKey_DisplayName;
	}
	if (!Child.TechnicalName.IsEmpty())
	{
		TechnicalName = Child.TechnicalName;
	}
	if (!Child.EnumValues.IsEmpty())
	{
		EnumValues = Child.EnumValues;
	}
	if (!Child.Features.IsEmpty())
	{
		Features = Child.Features;
	}
	if (!Child.Properties.IsEmpty())
	{
		Properties = Child.Properties;
	}
}

void FArticyType::MergeParent(const FArticyType& Parent)
{
	HasTemplate |= Parent.HasTemplate;
	IsEnum |= Parent.IsEnum;
	if (CPPType.IsEmpty())
	{
		CPPType = Parent.CPPType;
	}
	if (DisplayName.IsEmpty())
	{
		DisplayName = Parent.DisplayName;
	}
	if (LocaKey_DisplayName.IsEmpty())
	{
		LocaKey_DisplayName = Parent.LocaKey_DisplayName;
	}
	if (TechnicalName.IsEmpty())
	{
		TechnicalName = Parent.TechnicalName;
	}
	if (EnumValues.IsEmpty())
	{
		EnumValues = Parent.EnumValues;
	}
	if (Features.IsEmpty())
	{
		Features = Parent.Features;
	}
	if (Properties.IsEmpty())
	{
		Properties = Parent.Properties;
	}
}
