//		
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Customizations/Details/ArticyRefCustomization.h"
#include "ArticyFunctionLibrary.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "ArticyObject.h"
#include "ArticyRef.h"
#include "UObject/ConstructorHelpers.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "EditorCategoryUtils.h"
#include "Slate/SArticyRefProperty.h"

/**
 * @brief Creates a shared instance of FArticyRefCustomization.
 *
 * @return A shared pointer to a new instance of FArticyRefCustomization.
 */
TSharedRef<IPropertyTypeCustomization> FArticyRefCustomization::MakeInstance()
{
	return MakeShareable(new FArticyRefCustomization());
}

/**
 * @brief Customizes the header of the ArticyRef property in the details panel.
 *
 * This function sets up the custom UI elements for the ArticyRef property header.
 *
 * @param PropertyHandle The handle to the property being customized.
 * @param HeaderRow The header row where custom UI elements are added.
 * @param CustomizationUtils Utilities for property customization.
 */
void FArticyRefCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ArticyRefPropertyHandle = PropertyHandle;

	ArticyRefPropertyWidget = SNew(SArticyRefProperty)
		.ArticyRefToDisplay(this, &FArticyRefCustomization::GetArticyRef)
		.OnArticyRefChanged(this, &FArticyRefCustomization::OnArticyRefChanged)
		.TopLevelClassRestriction(this, &FArticyRefCustomization::GetClassRestriction)
		.bExactClass(IsExactClass())
		.bExactClassEditable(!HasExactClassMetaData())
		.bIsReadOnly(this, &FArticyRefCustomization::IsReadOnly);

	HeaderRow.NameContent()
		[
			ArticyRefPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(150)
		[
			ArticyRefPropertyWidget.ToSharedRef()
		];
}

/**
 * @brief Customizes the children of the ArticyRef property in the details panel.
 *
 * This function does not add any custom children to the ArticyRef property.
 *
 * @param PropertyHandle The handle to the property being customized.
 * @param ChildBuilder The builder for customizing the children.
 * @param CustomizationUtils Utilities for property customization.
 */
void FArticyRefCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Do not customize children
}

/**
 * @brief Retrieves the ArticyRef object from a given property handle.
 *
 * @param ArticyRefHandle The handle to the ArticyRef property.
 * @return A pointer to the ArticyRef object.
 */
FArticyRef* FArticyRefCustomization::RetrieveArticyRef(IPropertyHandle* ArticyRefHandle)
{
	FArticyRef* ArticyRef = nullptr;
	void* ArticyRefAddress;
	ArticyRefHandle->GetValueData(ArticyRefAddress);
	ArticyRef = static_cast<FArticyRef*>(ArticyRefAddress);
	return ArticyRef;
}

/**
 * @brief Retrieves the current ArticyRef object from the property handle.
 *
 * @return The current ArticyRef object.
 */
FArticyRef FArticyRefCustomization::GetArticyRef() const
{
	FArticyRef* ArticyRef = RetrieveArticyRef(ArticyRefPropertyHandle.Get());
	return ArticyRef ? *ArticyRef : FArticyRef();
}

/**
 * @brief Callback function for when the ArticyRef changes.
 *
 * This function updates the ArticyRef with the new ID using Unreal's property system.
 *
 * @param NewArticyRef The new ArticyRef object.
 */
void FArticyRefCustomization::OnArticyRefChanged(const FArticyRef& NewArticyRef) const
{
	// Update the ArticyRef with the new ID using Unreal's property system
	ArticyRefPropertyHandle->SetValueFromFormattedString(NewArticyRef.ToString());
}

/**
 * @brief Retrieves the class restriction for the ArticyRef property.
 *
 * This function checks metadata for class restrictions and attempts to find the class.
 *
 * @return The class restriction for the ArticyRef property.
 */
UClass* FArticyRefCustomization::GetClassRestriction() const
{
	UClass* Restriction = nullptr;

	if (HasClassRestrictionMetaData())
	{
		const FString ArticyClassRestriction = ArticyRefPropertyHandle->GetMetaData("ArticyClassRestriction");

		auto FullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), TEXT("ArticyRuntime"), *ArticyClassRestriction);
		Restriction = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, UArticyObject::StaticClass());

		// The class name can be in the ArticyRuntime module or in the project module
		if (Restriction == nullptr)
		{
			FullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), FApp::GetProjectName(), *ArticyClassRestriction);
			Restriction = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, UArticyObject::StaticClass());
		}
	}

	if (Restriction == nullptr)
	{
		Restriction = UArticyObject::StaticClass();
	}

	return Restriction;
}

/**
 * @brief Checks if the property has class restriction metadata.
 *
 * @return True if class restriction metadata exists, false otherwise.
 */
bool FArticyRefCustomization::HasClassRestrictionMetaData() const
{
	return ArticyRefPropertyHandle->HasMetaData(TEXT("ArticyClassRestriction"));
}

/**
 * @brief Checks if the ArticyRef is restricted to an exact class.
 *
 * @return True if the ArticyRef requires an exact class, false otherwise.
 */
bool FArticyRefCustomization::IsExactClass() const
{
	if (HasExactClassMetaData())
	{
		return ArticyRefPropertyHandle->GetBoolMetaData(TEXT("ArticyExactClass"));
	}

	return false;
}

/**
 * @brief Checks if the ArticyRef property is read-only.
 *
 * @return True if the property is read-only, false otherwise.
 */
bool FArticyRefCustomization::IsReadOnly() const
{
	return ArticyRefPropertyHandle->GetNumPerObjectValues() != 1 || ArticyRefPropertyHandle->IsEditConst();
}

/**
 * @brief Checks if the property has exact class metadata.
 *
 * @return True if exact class metadata exists, false otherwise.
 */
bool FArticyRefCustomization::HasExactClassMetaData() const
{
	return ArticyRefPropertyHandle->HasMetaData(TEXT("ArticyExactClass"));
}

/**
 * @brief Retrieves the ArticyId from a formatted value string.
 *
 * @param SourceString The formatted value string containing the ArticyId.
 * @return The ArticyId extracted from the value string.
 */
FArticyId FArticyRefCustomization::GetIdFromValueString(FString SourceString)
{
	int32 Low, High = 0;
	const bool bSuccess = FParse::Value(*SourceString, TEXT("Low="), Low) && FParse::Value(*SourceString, TEXT("High="), High);

	FArticyId Id;
	Id.High = High;
	Id.Low = Low;
	return Id;
}
