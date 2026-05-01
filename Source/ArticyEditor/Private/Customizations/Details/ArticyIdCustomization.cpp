//		
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Customizations/Details/ArticyIdCustomization.h"
#include "ArticyFunctionLibrary.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "ArticyObject.h"
#include "UObject/ConstructorHelpers.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "EditorCategoryUtils.h"

/**
 * @brief Constructs an instance of FArticyRefClassFilter with specified class and exact class requirement.
 *
 * @param InGivenClass The class to filter by.
 * @param bInRequiresExactClass Whether an exact class match is required.
 */
FArticyRefClassFilter::FArticyRefClassFilter(UClass* InGivenClass, bool bInRequiresExactClass) : GivenClass(InGivenClass), bRequiresExactClass(bInRequiresExactClass)
{
}

/**
 * @brief Creates a shared instance of the FArticyIdCustomization.
 *
 * @return A shared pointer to a new instance of FArticyIdCustomization.
 */
TSharedRef<IPropertyTypeCustomization> FArticyIdCustomization::MakeInstance()
{
	return MakeShareable(new FArticyIdCustomization());
}

/**
 * @brief Customizes the header of the Articy ID property in the details panel.
 *
 * This function sets up the custom widget for the Articy ID property.
 *
 * @param PropertyHandle The handle to the property being customized.
 * @param HeaderRow The row in the details panel to customize.
 * @param CustomizationUtils Utilities for property type customization.
 */
void FArticyIdCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ArticyIdPropertyHandle = PropertyHandle;

	if (ArticyIdPropertyHandle->HasMetaData("ArticyNoWidget"))
	{
		bShouldCustomize = !ArticyIdPropertyHandle->GetBoolMetaData("ArticyNoWidget");
	}

	if (!bShouldCustomize)
	{
		HeaderRow.NameContent()
			[
				ArticyIdPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				ArticyIdPropertyHandle->CreatePropertyValueWidget()
			];
		return;
	}

	ArticyIdPropertyWidget = SNew(SArticyIdProperty)
		.ArticyIdToDisplay(this, &FArticyIdCustomization::GetArticyId)
		.OnArticyIdChanged(this, &FArticyIdCustomization::OnArticyIdChanged)
		.TopLevelClassRestriction(this, &FArticyIdCustomization::GetClassRestriction)
		.bExactClass(IsExactClass())
		.bExactClassEditable(!HasExactClassMetaData())
		.bIsReadOnly(this, &FArticyIdCustomization::IsReadOnly);

	HeaderRow.NameContent()
		[
			ArticyIdPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(150)
		[
			ArticyIdPropertyWidget.ToSharedRef()
		];
}

/**
 * @brief Customizes the children of the Articy ID property in the details panel.
 *
 * This function is currently not implemented and does not customize any children.
 *
 * @param PropertyHandle The handle to the property being customized.
 * @param ChildBuilder The builder for customizing child properties.
 * @param CustomizationUtils Utilities for property type customization.
 */
void FArticyIdCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// dont do children
}

/**
 * @brief Retrieves the Articy ID from the specified property handle.
 *
 * This function extracts the Articy ID from the property handle's data.
 *
 * @param ArticyIdHandle The handle to the Articy ID property.
 * @return A pointer to the retrieved Articy ID.
 */
FArticyId* FArticyIdCustomization::RetrieveArticyId(IPropertyHandle* ArticyIdHandle)
{
	FArticyId* ArticyId = nullptr;
	void* ArticyRefAddress;
	ArticyIdHandle->GetValueData(ArticyRefAddress);
	ArticyId = static_cast<FArticyId*>(ArticyRefAddress);
	return ArticyId;
}

/**
 * @brief Gets the current Articy ID from the property handle.
 *
 * @return The current Articy ID.
 */
FArticyId FArticyIdCustomization::GetArticyId() const
{
	FArticyId* ArticyId = RetrieveArticyId(ArticyIdPropertyHandle.Get());
	return ArticyId ? *ArticyId : FArticyId();
}

/**
 * @brief Updates the Articy ID property when the ID changes.
 *
 * This function sets the new Articy ID in the property handle, handling various Unreal logic such as marking dirty and transaction buffer.
 *
 * @param NewArticyId The new Articy ID.
 */
void FArticyIdCustomization::OnArticyIdChanged(const FArticyId& NewArticyId) const
{
	// update the articy ref with the new ID:
	// done via Set functions instead of accessing the ref object directly because using "Set" handles various Unreal logic, such as:
	// - CDO default change forwarding to instances
	// - marking dirty
	// - transaction buffer (Undo, Redo)
	ArticyIdPropertyHandle->SetValueFromFormattedString(NewArticyId.ToString());
}

/**
 * @brief Gets the class restriction for the Articy ID property.
 *
 * This function returns the class restriction based on metadata or defaults to UArticyObject if no restriction is found.
 *
 * @return The class restriction for the Articy ID property.
 */
UClass* FArticyIdCustomization::GetClassRestriction() const
{
	UClass* Restriction = nullptr;

	if (HasClassRestrictionMetaData())
	{
		const FString ArticyClassRestriction = ArticyIdPropertyHandle->GetMetaData("ArticyClassRestriction");

		auto FullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), TEXT("ArticyRuntime"), *ArticyClassRestriction);
		Restriction = ConstructorHelpersInternal::FindOrLoadClass(FullClassName, UArticyObject::StaticClass());

		// the class name can be in the ArticyRuntime module or in the project module. If it wasn't found in ArticyRuntime, check the project module
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
 * @brief Checks if the Articy ID property has class restriction metadata.
 *
 * @return True if the class restriction metadata is present, false otherwise.
 */
bool FArticyIdCustomization::HasClassRestrictionMetaData() const
{
	return ArticyIdPropertyHandle->HasMetaData("ArticyClassRestriction");
}

/**
 * @brief Determines if the Articy ID requires an exact class match.
 *
 * @return True if an exact class match is required, false otherwise.
 */
bool FArticyIdCustomization::IsExactClass() const
{
	if (HasExactClassMetaData())
	{
		return ArticyIdPropertyHandle->GetBoolMetaData("ArticyExactClass");
	}

	return false;
}

/**
 * @brief Checks if the Articy ID property is read-only.
 *
 * @return True if the property is read-only, false otherwise.
 */
bool FArticyIdCustomization::IsReadOnly() const
{
	return ArticyIdPropertyHandle->GetNumPerObjectValues() != 1 || ArticyIdPropertyHandle->IsEditConst();
}

/**
 * @brief Checks if the Articy ID property has exact class metadata.
 *
 * @return True if the exact class metadata is present, false otherwise.
 */
bool FArticyIdCustomization::HasExactClassMetaData() const
{
	return ArticyIdPropertyHandle->HasMetaData("ArticyExactClass");
}

/**
 * @brief Parses the given string to extract the Articy ID.
 *
 * This function parses the source string to extract the low and high values of the Articy ID.
 *
 * @param SourceString The string to parse.
 * @return The parsed Articy ID.
 */
FArticyId FArticyIdCustomization::GetIdFromValueString(FString SourceString)
{
	int32 Low, High = 0;
	const bool bSuccess = FParse::Value(*SourceString, TEXT("Low="), Low) && FParse::Value(*SourceString, TEXT("High="), High);

	FArticyId Id;
	Id.High = High;
	Id.Low = Low;
	return Id;
}
