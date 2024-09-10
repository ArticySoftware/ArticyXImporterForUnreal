//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyType.h"
#include "ArticyHelpers.h"

FArticyEnumValueInfo FArticyType::GetEnumValue(int Value) const
{
	for (const auto& EnumInfo : EnumValues)
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
	for (const auto& EnumInfo : EnumValues)
	{
		if (EnumInfo.LocaKey_DisplayName.Equals(ValueName))
		{
			return EnumInfo;
		}
	}
	return {};
}

FString FArticyType::GetFeatureDisplayName(UObject* Outer, const FString& FeatureName) const
{
	return LocalizeString(Outer, FeatureName);
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
	// TODO: Implement this functionality
	return {};
}

FArticyPropertyInfo FArticyType::GetProperty(const FString& PropertyName) const
{
	for (const auto& PropertyInfo : Properties)
	{
		if (PropertyInfo.LocaKey_DisplayName.Equals(PropertyName))
		{
			return PropertyInfo;
		}
	}
	return {};
}

FString FArticyType::GetDisplayName(UObject* Outer)
{
	return LocalizeString(Outer, LocaKey_DisplayName);
}

FString FArticyType::LocalizeString(UObject* Outer, const FString& Input)
{
	return ArticyHelpers::LocalizeString(Outer, FText::FromString(Input)).ToString();
}

void FArticyType::MergeChild(const FArticyType& Child)
{
	HasTemplate |= Child.HasTemplate;
	IsEnum |= Child.IsEnum;
	if (!Child.CPPType.IsEmpty())
	{
		CPPType = Child.CPPType;
	}
	if (!Child.LocaKey_DisplayName.IsEmpty())
	{
		LocaKey_DisplayName = Child.LocaKey_DisplayName;
	}
	if (!Child.TechnicalName.IsEmpty())
	{
		TechnicalName = Child.TechnicalName;
	}
	if (Child.EnumValues.Num() > 0)
	{
		EnumValues = Child.EnumValues;
	}
	if (Child.Features.Num() > 0)
	{
		Features = Child.Features;
	}
	if (Child.Properties.Num() > 0)
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
	if (LocaKey_DisplayName.IsEmpty())
	{
		LocaKey_DisplayName = Parent.LocaKey_DisplayName;
	}
	if (TechnicalName.IsEmpty())
	{
		TechnicalName = Parent.TechnicalName;
	}
	if (EnumValues.Num() == 0)
	{
		EnumValues = Parent.EnumValues;
	}
	if (Features.Num() == 0)
	{
		Features = Parent.Features;
	}
	if (Properties.Num() == 0)
	{
		Properties = Parent.Properties;
	}
}
