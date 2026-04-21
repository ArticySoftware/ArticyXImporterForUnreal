//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Customizations/ArticyEditorCustomizationManager.h"
#include "ArticyObject.h"

/**
 * @brief Constructs an instance of FArticyEditorCustomizationManager.
 *
 * This constructor initializes the customization manager responsible for managing Articy editor customizations.
 */
FArticyEditorCustomizationManager::FArticyEditorCustomizationManager()
{
}

/**
 * @brief Registers a customization factory for ArticyIdPropertyWidget.
 *
 * This function registers a factory used to create customizations for ArticyIdPropertyWidget.
 *
 * @param GetCustomizationDelegate A delegate that returns a shared reference to an ArticyIdPropertyWidgetCustomizationFactory.
 * @return A pointer to the newly registered customization factory.
 */
IArticyIdPropertyWidgetCustomizationFactory* FArticyEditorCustomizationManager::RegisterArticyIdPropertyWidgetCustomizationFactory(FOnCreateArticyIdPropertyWidgetCustomizationFactory GetCustomizationDelegate)
{
	ArticyIdPropertyWidgetCustomizationFactories.Add(GetCustomizationDelegate.Execute());
	return ArticyIdPropertyWidgetCustomizationFactories.Last().Get();
}

/**
 * @brief Unregisters a customization factory for ArticyIdPropertyWidget.
 *
 * This function removes the specified customization factory from the list of registered factories.
 *
 * @param Factory A pointer to the customization factory to be unregistered.
 */
void FArticyEditorCustomizationManager::UnregisterArticyIdPropertyWidgetCustomizationFactory(const IArticyIdPropertyWidgetCustomizationFactory* Factory)
{
#if __cplusplus >= 202002L
	const int32 RemovedElements = ArticyIdPropertyWidgetCustomizationFactories.RemoveAll([=, this](const TSharedPtr<IArticyIdPropertyWidgetCustomizationFactory> FactoryPtr)
#else
	const int32 RemovedElements = ArticyIdPropertyWidgetCustomizationFactories.RemoveAll([=](const TSharedPtr<IArticyIdPropertyWidgetCustomizationFactory> FactoryPtr)
#endif
		{
			return Factory == FactoryPtr.Get();
		});

	ensureMsgf(RemovedElements != 0, TEXT("Failed removing factory. It was either not registered or removed already."));
}

/**
 * @brief Creates customizations for ArticyIdPropertyWidget based on the provided ArticyObject.
 *
 * This function generates customizations for the specified ArticyObject and adds them to the OutCustomizations array.
 *
 * @param ArticyObject A pointer to the ArticyObject for which customizations are being created.
 * @param OutCustomizations An array to hold the generated customizations.
 */
void FArticyEditorCustomizationManager::CreateArticyIdPropertyWidgetCustomizations(const UArticyObject* ArticyObject, TArray<TSharedPtr<IArticyIdPropertyWidgetCustomization>>& OutCustomizations)
{
	if (ArticyObject == nullptr)
	{
		return;
	}

	for (const TSharedPtr<IArticyIdPropertyWidgetCustomizationFactory>& Entry : ArticyIdPropertyWidgetCustomizationFactories)
	{
		if (Entry->SupportsType(ArticyObject))
		{
			OutCustomizations.Add(Entry->CreateCustomization());
		}
	}
}

/**
 * @brief Shuts down the customization manager.
 *
 * This function clears the list of registered customization factories.
 */
void FArticyEditorCustomizationManager::Shutdown()
{
	ArticyIdPropertyWidgetCustomizationFactories.Empty();
}
