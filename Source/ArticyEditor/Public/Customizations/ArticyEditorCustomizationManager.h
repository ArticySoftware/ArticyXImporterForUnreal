//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyRef.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

/** Architecture Ref: SequenceCustomizationManager */

/**
 * @struct FArticyIdPropertyWidgetCustomizationInfo
 * @brief Holds information for customizing an ArticyIdPropertyWidget.
 *
 * This struct contains a shared pointer to an FExtender, which allows for additional UI extensions.
 */
struct FArticyIdPropertyWidgetCustomizationInfo
{
	TSharedPtr<FExtender> ExtraButtonExtender;
};

/**
 * @class FArticyIdPropertyWidgetCustomizationBuilder
 * @brief Builder class for constructing ArticyIdPropertyWidget customizations.
 *
 * This class provides methods to add customizations and retrieve the customized ArticyObject.
 */
class FArticyIdPropertyWidgetCustomizationBuilder
{
public:
	/**
	 * @brief Constructs a new instance of FArticyIdPropertyWidgetCustomizationBuilder.
	 *
	 * @param InObject The ArticyObject to be customized.
	 */
	explicit FArticyIdPropertyWidgetCustomizationBuilder(const UArticyObject* InObject) : ArticyObject(InObject)
	{}

	/**
	 * @brief Adds a customization to the builder.
	 *
	 * @param Customization The customization information to add.
	 */
	void AddCustomization(const FArticyIdPropertyWidgetCustomizationInfo& Customization) { Customizations.Add(Customization); }

	/**
	 * @brief Retrieves the customizations added to the builder.
	 *
	 * @return An array of customization information.
	 */
	const TArray<FArticyIdPropertyWidgetCustomizationInfo>& GetCustomizations() const { return Customizations; }

	/**
	 * @brief Retrieves the ArticyObject being customized.
	 *
	 * @return A pointer to the ArticyObject.
	 */
	const UArticyObject* GetArticyObject() const { return ArticyObject; }

private:
	const UArticyObject* ArticyObject;
	TArray<FArticyIdPropertyWidgetCustomizationInfo> Customizations;
};

/**
 * @class IArticyIdPropertyWidgetCustomization
 * @brief Interface for customizing ArticyIdPropertyWidget.
 *
 * This interface defines methods for registering and unregistering customizations for ArticyIdPropertyWidget.
 */
class ARTICYEDITOR_API IArticyIdPropertyWidgetCustomization
{
public:
	virtual ~IArticyIdPropertyWidgetCustomization() {}

	/**
	 * @brief Registers an ArticyIdPropertyWidget customization.
	 *
	 * @param Builder The builder used to register customizations.
	 */
	virtual void RegisterArticyIdPropertyWidgetCustomization(FArticyIdPropertyWidgetCustomizationBuilder& Builder) = 0;

	/**
	 * @brief Unregisters an ArticyIdPropertyWidget customization.
	 */
	virtual void UnregisterArticyIdPropertyWidgetCustomization() = 0;
};

/**
 * @class IArticyIdPropertyWidgetCustomizationFactory
 * @brief Factory interface for creating ArticyIdPropertyWidget customizations.
 *
 * This interface defines methods for creating customizations and checking if a type is supported.
 */
class ARTICYEDITOR_API IArticyIdPropertyWidgetCustomizationFactory
{
public:
	virtual ~IArticyIdPropertyWidgetCustomizationFactory() {}

	/**
	 * @brief Creates a new ArticyIdPropertyWidget customization.
	 *
	 * @return A shared pointer to the created customization.
	 */
	virtual TSharedPtr<IArticyIdPropertyWidgetCustomization> CreateCustomization() = 0;

	/**
	 * @brief Checks if the factory supports the specified ArticyObject type.
	 *
	 * @param InObject The ArticyObject to check for support.
	 * @return True if the type is supported, false otherwise.
	 */
	virtual bool SupportsType(const UArticyObject* InObject) = 0;
};

/** Delegate for creating ArticyIdPropertyWidget customization factories. */
DECLARE_DELEGATE_RetVal(TSharedRef<IArticyIdPropertyWidgetCustomizationFactory>, FOnCreateArticyIdPropertyWidgetCustomizationFactory);

/**
 * @class FArticyEditorCustomizationManager
 * @brief Singleton-style class for managing Articy editor customizations.
 *
 * This class provides methods for registering, unregistering, and creating ArticyIdPropertyWidget customizations.
 */
class ARTICYEDITOR_API FArticyEditorCustomizationManager
{
public:
	/** Constructs an instance of FArticyEditorCustomizationManager. */
	FArticyEditorCustomizationManager();

	/**
	 * @brief Registers an ArticyIdPropertyWidget customization factory.
	 *
	 * @param GetCustomizationDelegate A delegate that returns a shared reference to an ArticyIdPropertyWidgetCustomizationFactory.
	 * @return A pointer to the newly registered customization factory.
	 */
	IArticyIdPropertyWidgetCustomizationFactory* RegisterArticyIdPropertyWidgetCustomizationFactory(FOnCreateArticyIdPropertyWidgetCustomizationFactory GetCustomizationDelegate);

	/**
	 * @brief Unregisters an ArticyIdPropertyWidget customization factory.
	 *
	 * @param IArticyIdPropertyWidgetCustomizationFactory A pointer to the customization factory to be unregistered.
	 */
	void UnregisterArticyIdPropertyWidgetCustomizationFactory(const IArticyIdPropertyWidgetCustomizationFactory*);

	/**
	 * @brief Creates the ArticyIdPropertyWidget customizations for an ArticyRef.
	 *
	 * @param ArticyObject A pointer to the ArticyObject for which customizations are being created.
	 * @param OutCustomizations An array to hold the generated customizations.
	 */
	void CreateArticyIdPropertyWidgetCustomizations(const UArticyObject* ArticyObject, TArray<TSharedPtr<IArticyIdPropertyWidgetCustomization>>& OutCustomizations);

private:
	/** Called during module shutdown to clear registered factories. */
	void Shutdown();

	/** Delete copy constructor and assignment operator to ensure a single instance. */
	FArticyEditorCustomizationManager(const FArticyEditorCustomizationManager&) = delete;
	FArticyEditorCustomizationManager& operator=(const FArticyEditorCustomizationManager&) = delete;

	TArray<TSharedPtr<IArticyIdPropertyWidgetCustomizationFactory>> ArticyIdPropertyWidgetCustomizationFactories;

	friend class FArticyEditorModule;
};
