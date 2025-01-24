//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include <IPropertyTypeCustomization.h>
#include "ArticyRef.h"
#include "ArticyObject.h"

/**
 * @class FArticyRefCustomization
 * @brief Customizes the ArticyRef property in the Unreal Engine editor's details panel.
 *
 * This class provides custom UI elements and functionality for editing ArticyRef properties, including class restrictions and value display.
 */
class FArticyRefCustomization final : public IPropertyTypeCustomization
{
public:
	/**
	 * @brief Creates a shared instance of FArticyRefCustomization.
	 *
	 * @return A shared pointer to a new instance of FArticyRefCustomization.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/**
	 * @brief Retrieves the ArticyRef object from a given property handle.
	 *
	 * @param ArticyIdHandle The handle to the ArticyRef property.
	 * @return A pointer to the ArticyRef object.
	 */
	static FArticyRef* RetrieveArticyRef(IPropertyHandle* ArticyIdHandle);

	/**
	 * @brief Retrieves the ArticyId from a formatted value string.
	 *
	 * @param SourceString The formatted value string containing the ArticyId.
	 * @return The ArticyId extracted from the value string.
	 */
	static FArticyId GetIdFromValueString(FString SourceString);

private:
	/** Handle to the ArticyRef property. */
	TSharedPtr<IPropertyHandle> ArticyRefPropertyHandle;

	/** Widget for the ArticyRef property. */
	TSharedPtr<SWidget> ArticyRefPropertyWidget;

	/**
	 * @brief Retrieves the class restriction for the ArticyRef property.
	 *
	 * @return The class restriction for the ArticyRef property.
	 */
	UClass* GetClassRestriction() const;

	/**
	 * @brief Checks if the ArticyRef is restricted to an exact class.
	 *
	 * @return True if the ArticyRef requires an exact class, false otherwise.
	 */
	bool IsExactClass() const;

	/**
	 * @brief Checks if the ArticyRef property is read-only.
	 *
	 * @return True if the property is read-only, false otherwise.
	 */
	bool IsReadOnly() const;

	/**
	 * @brief Checks if the property has class restriction metadata.
	 *
	 * @return True if class restriction metadata exists, false otherwise.
	 */
	bool HasClassRestrictionMetaData() const;

	/**
	 * @brief Checks if the property has exact class metadata.
	 *
	 * @return True if exact class metadata exists, false otherwise.
	 */
	bool HasExactClassMetaData() const;

	/**
	 * @brief Retrieves the current ArticyRef object from the property handle.
	 *
	 * @return The current ArticyRef object.
	 */
	FArticyRef GetArticyRef() const;

	/**
	 * @brief Callback function for when the ArticyRef changes.
	 *
	 * This function updates the ArticyRef with the new ID using Unreal's property system.
	 *
	 * @param NewArticyRef The new ArticyRef object.
	 */
	void OnArticyRefChanged(const FArticyRef& NewArticyRef) const;
};
