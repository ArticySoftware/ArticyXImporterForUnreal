//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyType.h"

#include "ArticyTypeSystem.h"

FArticyEnumValueInfo FArticyType::GetEnumValue(int Value) const
{
	for (const auto& EnumInfo : EnumValues)
	{
		if (EnumInfo.Value == Value)
			return EnumInfo;
	}
	return {};
}

FArticyEnumValueInfo FArticyType::GetEnumValue(const FString& ValueName) const
{
	for (const auto& EnumInfo : EnumValues)
	{
		if (EnumInfo.LocaKey_DisplayName.Equals(ValueName))
			return EnumInfo;
	}
	return {};
}

FArticyPropertyInfo FArticyType::GetProperty(const FString& PropertyName) const
{
	for (const auto& PropertyInfo : Properties)
	{
		if (PropertyInfo.TechnicalName.Equals(PropertyName) ||
			PropertyInfo.LocaKey_DisplayName.Equals(PropertyName))
			return PropertyInfo;
	}
	FArticyPropertyInfo InvalidProperty;
	InvalidProperty.IsInvalidProperty = true;
	return InvalidProperty;
}

FString FArticyType::GetFeatureDisplayName(const FString& FeatureName)
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

void FArticyType::MergeProperties(const FArticyType& Other, bool isChild)
{
	HasTemplate |= Other.HasTemplate;
	IsEnum |= Other.IsEnum;

	#define MERGE_IF_EMPTY(field) if (field.IsEmpty()) field = Other.field;
	#define MERGE_IF_NOT_EMPTY(field) if (!Other.field.IsEmpty()) field = Other.field;

	if(isChild)
	{
		MERGE_IF_NOT_EMPTY(CPPType);
		MERGE_IF_NOT_EMPTY(DisplayName);
		MERGE_IF_NOT_EMPTY(LocaKey_DisplayName);
		MERGE_IF_NOT_EMPTY(TechnicalName);
		MERGE_IF_NOT_EMPTY(EnumValues);
		MERGE_IF_NOT_EMPTY(Features);
		MERGE_IF_NOT_EMPTY(Properties);
	}
	else
	{
		MERGE_IF_EMPTY(CPPType);
		MERGE_IF_EMPTY(DisplayName);
		MERGE_IF_EMPTY(LocaKey_DisplayName);
		MERGE_IF_EMPTY(TechnicalName);
		MERGE_IF_EMPTY(EnumValues);
		MERGE_IF_EMPTY(Features);
		MERGE_IF_EMPTY(Properties);
	}

	#undef MERGE_IF_EMPTY
	#undef MERGE_IF_NOT_EMPTY
}

void FArticyType::MergeChild(const FArticyType& Child)
{
	MergeProperties(Child, true);
}

void FArticyType::MergeParent(const FArticyType& Parent)
{
	MergeProperties(Parent, false);
}
