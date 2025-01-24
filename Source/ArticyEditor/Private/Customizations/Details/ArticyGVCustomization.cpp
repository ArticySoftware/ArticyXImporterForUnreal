//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Customizations/Details/ArticyGVCustomization.h"
#include "ArticyGlobalVariables.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Delegates/Delegate.h"
#include "ArticyEditorModule.h"
#include "DetailLayoutBuilder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Slate/GV/SArticyGlobalVariablesDebugger.h"
#include "Runtime/Launch/Resources/Version.h"

/**
 * @brief Customizes the details panel for Articy Global Variables.
 *
 * This function customizes the details panel for a single ArticyGlobalVariables object, hiding the default categories and displaying a custom UI.
 *
 * @param DetailBuilder The detail layout builder used to customize the details panel.
 */
void FArticyGVCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> WeakObjects;
	DetailBuilder.GetObjectsBeingCustomized(WeakObjects);

	if (WeakObjects.Num() != 1) return;

	TArray<UObject*> Objects;
	CopyFromWeakArray(Objects, WeakObjects);

	UObject* Object = Objects[0];
	UArticyGlobalVariables* GV = Cast<UArticyGlobalVariables>(Object);

	TMap<FString, TSharedRef<IPropertyHandle>> Properties;
	TArray<FName> CategoryNames;
	// @TODO GetCategoryNames was introduced in 4.22.
	// Since the details panel should generally never be seen, it's okay that other categories are not hidden in < 4.22
#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 22)
	DetailBuilder.GetCategoryNames(CategoryNames);
#endif

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(TEXT("Default"));

	// Hide all normal categories as we'll replace them with a custom UI
	for (FName CatName : CategoryNames)
	{
		DetailBuilder.HideCategory(CatName);
	}

	FDetailWidgetRow& Row = CategoryBuilder.AddCustomRow(FText::FromString(TEXT("Articy")));
	Row.WholeRowWidget
		[
			SNew(SArticyGlobalVariables, GV).bInitiallyCollapsed(true)
		];

	//// retrieve the propertyhandles for the properties in the class (which are variablesets), and create widgets for them
	//for(TFieldIterator<FObjectProperty> SetIt(Object->GetClass()); SetIt; ++SetIt)
	//{
	//	UE_LOG(LogArticyEditor, Warning, TEXT("%s"), *SetIt->GetName());
	//	if(Properties.Contains(*SetIt->GetName()))
	//	{
	//		TSharedRef<IPropertyHandle> Handle = Properties[*SetIt->GetName()];
	//		UObject* VarSetObject = nullptr;
	//		Handle->GetValue(VarSetObject);

	//		UArticyBaseVariableSet* VarSet = Cast<UArticyBaseVariableSet>(VarSetObject);
	//		ensure(VarSet);
	//		
	//		TSharedRef<SArticyVariableSet> VarSetWidget = SNew(SArticyVariableSet, VarSet)
	//		.SizeData(&SizeData);
	//		
	//		FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(Handle->GetPropertyDisplayName());
	//		WidgetRow
	//		[
	//			VarSetWidget
	//		];
	//	}
	//}
}

/**
 * @brief Creates an instance of the ArticyGVCustomization.
 *
 * This function creates and returns a shared pointer to a new instance of the ArticyGVCustomization class.
 *
 * @return A shared pointer to a new instance of the ArticyGVCustomization class.
 */
TSharedRef<IDetailCustomization> FArticyGVCustomization::MakeInstance()
{
	return MakeShareable(new FArticyGVCustomization());
}
