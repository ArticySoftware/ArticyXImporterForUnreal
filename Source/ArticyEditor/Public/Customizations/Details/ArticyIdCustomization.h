//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include <IPropertyTypeCustomization.h>
#include "Slate/SArticyIdProperty.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "ArticyObject.h"

/**
 * @class FArticyRefClassFilter
 * @brief Filters classes based on specified criteria for Articy references.
 *
 * This class provides filtering functionality to restrict class selections to a given class and its children or require an exact class match.
 */
class FArticyRefClassFilter final : public IClassViewerFilter
{
public:
	/**
	 * @brief Constructs an instance of FArticyRefClassFilter with specified class and exact class requirement.
	 *
	 * @param GivenClass The class to filter by.
	 * @param bInRequiresExactClass Whether an exact class match is required.
	 */
	explicit FArticyRefClassFilter(UClass* GivenClass = UArticyObject::StaticClass(), bool bInRequiresExactClass = false);

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if (bRequiresExactClass)
		{
			return InClass == GivenClass;
		}

		return InClass->IsChildOf(GivenClass);
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return false;
	}

	UClass* GivenClass = nullptr; ///< The class to filter by.
	bool bRequiresExactClass = false; ///< Whether an exact class match is required.
};

/**
 * @class FArticyIdCustomization
 * @brief Customizes the display and behavior of Articy ID properties in the details panel.
 *
 * This class provides customization for the Articy ID properties, including custom widgets and behavior for displaying and editing Articy IDs.
 */
class FArticyIdCustomization final : public IPropertyTypeCustomization
{
public:
	/**
	 * @brief Creates a shared instance of the FArticyIdCustomization.
	 *
	 * @return A shared pointer to a new instance of FArticyIdCustomization.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/**
	 * @brief Retrieves the Articy ID from the specified property handle.
	 *
	 * @param ArticyIdHandle The handle to the Articy ID property.
	 * @return A pointer to the retrieved Articy ID.
	 */
	static FArticyId* RetrieveArticyId(IPropertyHandle* ArticyIdHandle);

	/**
	 * @brief Parses the given string to extract the Articy ID.
	 *
	 * This function parses the source string to extract the low and high values of the Articy ID.
	 *
	 * @param SourceString The string to parse.
	 * @return The parsed Articy ID.
	 */
	static FArticyId GetIdFromValueString(FString SourceString);

private:
	TSharedPtr<IPropertyHandle> ArticyIdPropertyHandle; ///< Handle to the Articy ID property.
	TSharedPtr<SArticyIdProperty> ArticyIdPropertyWidget; ///< Widget for displaying and editing the Articy ID.
	bool bShouldCustomize = true; ///< Whether customization is enabled for the Articy ID.

	UClass* GetClassRestriction() const;
	bool IsExactClass() const;
	bool IsReadOnly() const;
	bool HasClassRestrictionMetaData() const;
	bool HasExactClassMetaData() const;

	FArticyId GetArticyId() const;
	void OnArticyIdChanged(const FArticyId& NewArticyId) const;
};
