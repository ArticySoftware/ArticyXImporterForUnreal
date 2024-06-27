//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyType.generated.h"

USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyEnumValueInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString LocaKey_DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString TechnicalName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	int Value = 0;
};

USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyPropertyInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TMap<int, FText> Constraints;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	bool IsTemplateProperty = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString LocaKey_DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString PropertyType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString TechnicalName;
};

USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyType
{
	GENERATED_BODY()

public:
	FArticyEnumValueInfo GetEnumValue(int Value) const;
	FArticyEnumValueInfo GetEnumValue(const FString& ValueName) const;
	FString GetFeatureDisplayName(UObject* Outer, const FString& FeatureName) const;
	FString GetFeatureDisplayNameLocaKey(const FString& FeatureName) const;
	TArray<FArticyPropertyInfo> GetProperties() const;
	TArray<FArticyPropertyInfo> GetPropertiesInFeature(const FString& FeatureName) const;
	FArticyPropertyInfo GetProperty(const FString& PropertyName) const;
	static FString LocalizeString(UObject* Outer, const FString& Input);
	FString GetDisplayName(UObject* WorldContext);

	void MergeChild(const FArticyType& Child);
	void MergeParent(const FArticyType& Parent);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString CPPType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TArray<FArticyEnumValueInfo> EnumValues;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TArray<FString> Features;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	bool HasTemplate = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	bool IsEnum = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString LocaKey_DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TArray<FArticyPropertyInfo> Properties;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	FString TechnicalName;
};
