//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyJSONFactory.h"
#include "PredefinedTypes.h"
#include "ArticyObject.h"
#include "ArticyTexts.h"

#include "ObjectDefinitionsImport.generated.h"

class UArticyObject;
struct FArticyModelDef;
class CodeFileGenerator;
struct FArticyObjectDefinitions;

/**
 * Represents a template constraint in Articy.
 */
USTRUCT()
struct FArticyTemplateConstraint
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "TemplateConstraint")
    FString Property = "";
    UPROPERTY(VisibleAnywhere, Category = "TemplateConstraint")
    FString Type = "";

    /** Only present on localizable properties. */
    UPROPERTY(VisibleAnywhere, Category = "TemplateConstraint")
    bool IsLocalized = false;

    /**
     * Import constraint data from a JSON object.
     *
     * @param JsonProperty A shared pointer to the JSON object containing the constraint data.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject> JsonProperty);
};

/**
 * Represents a property definition in Articy.
 */
USTRUCT()
struct FArticyPropertyDef
{
    GENERATED_BODY()

public:
    /**
     * Import property definition data from a JSON object.
     *
     * @param JsonProperty A shared pointer to the JSON object containing the property definition.
     * @param Data A pointer to the UArticyImportData object.
     * @param OptionalConstraints An optional array of template constraints.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject> JsonProperty, UArticyImportData* Data, const TArray<FArticyTemplateConstraint>* OptionalConstraints = nullptr);

    /**
     * Generates code for the property definition using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GenerateCode(CodeFileGenerator& header, UArticyImportData* Data) const;

    /**
     * Gathers script fragments from JSON and adds them to the UArticyImportData.
     *
     * @param JsonValue A shared pointer to the JSON object containing the script.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GatherScript(const TSharedPtr<FJsonObject>& JsonValue, UArticyImportData* Data) const;

    /**
     * Initializes a model with data from JSON.
     *
     * @param Model A pointer to the UArticyBaseObject.
     * @param Path The path of the model.
     * @param JsonValue A shared pointer to the JSON object containing the data.
     * @param Data A pointer to the UArticyImportData object.
     * @param PackageName The name of the package.
     */
    void InitializeModel(
        UArticyBaseObject* Model,
        const FString& Path,
        const TSharedPtr<FJsonObject>& JsonValue,
        const UArticyImportData* Data,
        const FString& PackageName) const;

    /**
     * Returns the name of the property.
     *
     * @return The property name as an FName.
     */
    const FName& GetPropetyName() const { return Property; }

    /**
     * Returns the original type of the property.
     *
     * @return The original type as an FName.
     */
    const FName& GetOriginalType() const { return Type; }

    /**
     * Returns the original item type of the property.
     *
     * @return The original item type as an FName.
     */
    const FName& GetOriginalItemType() const { return ItemType; }

    /**
     * Returns the C++ type of the property definition.
     *
     * @param Data A pointer to the UArticyImportData object.
     * @return The C++ type as a string.
     */
    FString GetCppType(UArticyImportData* Data) const;

private:
    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FName Property = "";
    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FName Type = "";

    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FName ItemType = "";
    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FString DisplayName = "";
    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FString Tooltip = "";

    UPROPERTY(VisibleAnywhere, Category = "ObjectProperty")
    FArticyType ArticyType;

    friend class UArticyImportData;
};

/**
 * Represents an enum value in Articy.
 */
USTRUCT()
struct FArticyEnumValue
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "EnumValue")
    FString Name = "";
    UPROPERTY(VisibleAnywhere, Category = "EnumValue")
    uint8 Value = 0; //0..255, limited by UE!

    /**
     * Imports enum value data from a JSON key-value pair.
     *
     * @param JsonKeyValue A key-value pair containing the enum value data.
     */
    void ImportFromJson(const TPair<FString, TSharedPtr<FJsonValue>> JsonKeyValue);
};

/**
 * Represents a template feature definition in Articy.
 */
USTRUCT()
struct FArticyTemplateFeatureDef
{
    GENERATED_BODY()

public:
    /**
     * Import template feature definition data from a JSON object.
     *
     * @param JsonObject A shared pointer to the JSON object containing the feature definition.
     * @param Data A pointer to the UArticyImportData object.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject> JsonObject, UArticyImportData* Data);

    /**
     * Generates definition code for the template feature using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GenerateDefCode(CodeFileGenerator& header, UArticyImportData* Data) const;

    /**
     * Generates property code for the template feature using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GeneratePropertyCode(CodeFileGenerator& header, const UArticyImportData* Data) const;

    /**
     * Gather scripts from JSON and adds them to the UArticyImportData.
     *
     * @param Json A shared pointer to the JSON object containing the scripts.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GatherScripts(const TSharedPtr<FJsonObject>& Json, UArticyImportData* Data) const;

    /**
     * Initializes a model with data from JSON.
     *
     * @param Model A pointer to the UArticyPrimitive object.
     * @param Path The path of the model.
     * @param Json A shared pointer to the JSON object containing the data.
     * @param Data A pointer to the UArticyImportData object.
     * @param PackageName The name of the package.
     */
    void InitializeModel(
        UArticyPrimitive* Model,
        const FString& Path,
        const TSharedPtr<FJsonObject>& Json,
        const UArticyImportData* Data,
        const FString& PackageName) const;

    /**
     * Returns the C++ type of the template feature.
     *
     * @param Data A pointer to the UArticyImportData object.
     * @param bAsVariable Whether the type is for a variable.
     * @return The C++ type as a string.
     */
    FString GetCppType(const UArticyImportData* Data, bool bAsVariable) const;

    /**
     * Returns the UClass of the template feature.
     *
     * @param Data A pointer to the UArticyImportData object.
     * @return A pointer to the UClass of the feature.
     */
    UClass* GetUClass(const UArticyImportData* Data) const;

    /**
     * Returns the technical name of the template feature.
     *
     * @return The technical name as a string.
     */
    FString GetTechnicalName() const { return TechnicalName; }

    /**
     * Returns the display name of the template feature.
     *
     * @return The display name as a string.
     */
    FString GetDisplayName() const { return DisplayName; }

private:
    UPROPERTY(VisibleAnywhere, Category = "TemplateFeature")
    FString TechnicalName = "";
    UPROPERTY(VisibleAnywhere, Category = "TemplateFeature")
    FString DisplayName = "";

    UPROPERTY(VisibleAnywhere, Category = "TemplateFeature")
    TArray<FArticyPropertyDef> Properties;

    UPROPERTY(VisibleAnywhere, Category = "TemplateFeature")
    TArray<FArticyTemplateConstraint> Constraints;

    UPROPERTY(VisibleAnywhere, Category = "TemplateFeature")
    FArticyType ArticyType;
};

/**
 * Represents a template definition in Articy.
 */
USTRUCT()
struct FArticyTemplateDef
{
    GENERATED_BODY()

public:
    /**
     * Import template definition data from a JSON object.
     *
     * @param JsonObject A shared pointer to the JSON object containing the template definition.
     * @param Data A pointer to the UArticyImportData object.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject> JsonObject, UArticyImportData* Data);

    /**
     * Generates feature definitions in the header file using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GenerateFeaturesDefs(CodeFileGenerator& header, UArticyImportData* Data) const;

    /**
     * Generates property definitions in the header file using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GenerateProperties(CodeFileGenerator& header, const UArticyImportData* Data) const;

    /**
     * Gather scripts from JSON values and adds them to the ArticyImportData.
     *
     * @param Values A shared pointer to the JSON object containing the values.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GatherScripts(const TSharedPtr<FJsonObject> Values, UArticyImportData* Data) const;

    /**
     * Initializes a model with data from JSON values.
     *
     * @param Model A pointer to the UArticyPrimitive object.
     * @param Path The path of the model.
     * @param Values A shared pointer to the JSON object containing the values.
     * @param Data A pointer to the UArticyImportData object.
     * @param PackageName The name of the package.
     */
    void InitializeModel(
        UArticyPrimitive* Model,
        const FString& Path,
        const TSharedPtr<FJsonObject> Values,
        const UArticyImportData* Data,
        const FString& PackageName) const;

    /**
     * Returns the display name of the template.
     *
     * @return The display name as a string.
     */
    FString GetDisplayName() const { return DisplayName; }

    /**
     * Returns the features of the template.
     *
     * @return A constant reference to the array of features.
     */
    const TArray<FArticyTemplateFeatureDef>& GetFeatures() const { return Features; }

    UPROPERTY(VisibleAnywhere, Category = "Template")
    FArticyType ArticyType;

private:
    UPROPERTY(VisibleAnywhere, Category = "Template")
    FString TechnicalName = "";
    UPROPERTY(VisibleAnywhere, Category = "Template")
    FString DisplayName = "";

    UPROPERTY(VisibleAnywhere, Category = "Template")
    TArray<FArticyTemplateFeatureDef> Features;

};

//---------------------------------------------------------------------------//

/**
 * Enumeration representing object definition types in Articy.
 */
UENUM()
enum class EObjectDefType : uint8
{
    Model, //just a Type with properties, no Template
    Enum,
    Template,
};

/**
 * Represents an object definition in Articy.
 */
USTRUCT()
struct FArticyObjectDef
{
    GENERATED_BODY()

public:
    /**
     * Imports object definition data from a JSON object.
     *
     * @param JsonObjDef A shared pointer to the JSON object containing the object definition.
     * @param Data A pointer to the UArticyImportData object.
     */
    void ImportFromJson(const TSharedPtr<FJsonObject> JsonObjDef, UArticyImportData* Data);

    /**
     * Checks if a property is already defined in the base class.
     *
     * @param Property The name of the property.
     * @param Data A pointer to the UArticyImportData object.
     * @return True if the property is a base property, false otherwise.
     */
    bool IsBaseProperty(FName Property, UArticyImportData* Data) const;

    /**
     * Generates code for the object definition using CodeFileGenerator.
     *
     * @param header A reference to the CodeFileGenerator.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GenerateCode(CodeFileGenerator& header, UArticyImportData* Data) const;

    /**
     * Find all script fragments, add them to the UArticyImportData, and replace them with an id.
     *
     * @param Values The model definition containing the values.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GatherScripts(const FArticyModelDef& Values, UArticyImportData* Data) const;

    /**
     * Initializes a model with data from the model definition.
     *
     * @param Model A pointer to the UArticyPrimitive object.
     * @param Values The model definition containing the values.
     * @param Data A pointer to the UArticyImportData object.
     * @param PackageName The name of the package.
     */
    void InitializeModel(
        UArticyPrimitive* Model,
        const FArticyModelDef& Values,
        UArticyImportData* Data,
        const FString& PackageName) const;

    /**
     * Returns the C++ type of the object definition.
     *
     * @param Data A pointer to the UArticyImportData object.
     * @param bForProperty Whether the type is for a property.
     * @return The C++ type as a string.
     */
    FString GetCppType(const UArticyImportData* Data, const bool bForProperty) const;

    /**
     * Returns the C++ base classes of the object definition.
     *
     * @param Data A pointer to the UArticyImportData object.
     * @return The C++ base classes as a string.
     */
    FString GetCppBaseClasses(UArticyImportData* Data) const;

    /**
     * Returns the original type of the object definition.
     *
     * @return The original type as an FName.
     */
    const FName& GetOriginalType() const { return Type; }

    /**
     * Returns the features of the object definition.
     *
     * @return A constant reference to the array of features.
     */
    const TArray<FArticyTemplateFeatureDef>& GetFeatures() const;

    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    FArticyType ArticyType;

private:
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    EObjectDefType DefType = EObjectDefType::Enum;

    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    FName Type = "";
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    FName Class = "";
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    FName InheritsFrom = "";

    /** Properties of the model. */
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    TArray<FArticyPropertyDef> Properties;
    /** Might be empty. */
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    FArticyTemplateDef Template;

    /** Only for enums. */
    UPROPERTY(VisibleAnywhere, Category = "ObjectDef")
    TArray<FArticyEnumValue> Values;
};

/**
 * Represents object definitions in Articy.
 */
USTRUCT()
struct FArticyObjectDefinitions
{
    GENERATED_BODY()

public:
    /**
     * Imports object definitions from a JSON array.
     *
     * @param Json A pointer to the array of JSON values containing the object definitions.
     * @param Data A pointer to the UArticyImportData object.
     */
    void ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json, UArticyImportData* Data);

    /**
     * Gather scripts from model definition and adds them to the UArticyImportData.
     *
     * @param Values The model definition containing the values.
     * @param Data A pointer to the UArticyImportData object.
     */
    void GatherScripts(const FArticyModelDef& Values, UArticyImportData* Data) const;

    /**
     * Gather text definitions from a JSON object.
     *
     * @param Json A shared pointer to the JSON object containing the text definitions.
     */
    void GatherText(const TSharedPtr<FJsonObject>& Json);

    /**
     * Initializes a model with data from the model definition.
     *
     * @param Model A pointer to the UArticyPrimitive object.
     * @param Values The model definition containing the values.
     * @param Data A pointer to the UArticyImportData object.
     * @param PackageName The name of the package.
     */
    void InitializeModel(UArticyPrimitive* Model, const FArticyModelDef& Values, UArticyImportData* Data, const FString& PackageName) const;

    /**
     * Returns the C++ type of an object definition.
     *
     * @param OriginalType The original type of the object definition.
     * @param Data A pointer to the UArticyImportData object.
     * @param bForProperty Whether the type is for a property.
     * @return The C++ type as a string.
     */
    FString GetCppType(const FName& OriginalType, const UArticyImportData* Data, const bool bForProperty) const;

    /**
     * Returns the default C++ value for a given type.
     *
     * @param OriginalType The original type.
     * @return The default value as a string.
     */
    static const FString& GetCppDefaultValue(const FName& OriginalType);

    /**
     * Contains information about a class in Articy.
     */
    struct FClassInfo
    {
        FClassInfo() = default;
        FClassInfo(const FString& Name, UClass* Class) : CppTypeName(Name), StaticClass(Class) {}

        FString CppTypeName;
        UClass* StaticClass;
    };

    /**
     * Returns the default base class for an object definition.
     *
     * @param OriginalType The original type of the object definition.
     * @param Data A pointer to the UArticyImportData object.
     * @return A reference to the FClassInfo struct containing the base class information.
     */
    static const FClassInfo& GetDefaultBaseClass(const FName& OriginalType, const UArticyImportData* Data);

    /**
     * Returns the provider interface name for a given property.
     *
     * @param Property The property definition.
     * @return The provider interface name as an FName.
     */
    static const FName& GetProviderInterface(const FArticyPropertyDef& Property);

    /**
     * Sets a property value on a model.
     *
     * @param OriginalType The original type of the property.
     * @param Property The name of the property.
     * @param PROP_SETTER_PARAMS Parameters for setting the property.
     */
    static void SetProp(const FName& OriginalType, const FName& Property, PROP_SETTER_PARAMS);

    /**
     * Checks if a feature type is new.
     *
     * @param CppType The C++ type of the feature.
     * @return True if the feature type is new, false otherwise.
     */
    bool IsNewFeatureType(const FName& CppType) const;

    /**
     * Returns the types map.
     *
     * @return A reference to the types map.
     */
    TMap<FName, FArticyObjectDef>& GetTypes() { return Types; }

    /**
     * Returns the types map.
     *
     * @return A constant reference to the types map.
     */
    const TMap<FName, FArticyObjectDef>& GetTypes() const { return Types; }

    /**
     * Returns the texts map.
     *
     * @return A reference to the texts map.
     */
    TMap<FString, FArticyTexts>& GetTexts() { return Texts; }

    /**
     * Returns the feature definitions map.
     *
     * @return A constant reference to the feature definitions map.
     */
    const TMap<FName, FArticyTemplateFeatureDef> GetFeatures() const { return FeatureDefs; }

private:
    /**
     * This stores information about all the types that have been imported.
     * Note that FName is case-insensitive, as is the type information in the
     * articy json export.
     */
    UPROPERTY(VisibleAnywhere, Category = "ObjectDefinitions")
    TMap<FName, FArticyObjectDef> Types;

    UPROPERTY(VisibleAnywhere, Category = "ObjectDefinitions")
    TMap<FString, FArticyTexts> Texts;

    /**
     * Contains the CPP type of all defined features.
     * This is needed to prevent feature types from being imported multiple times (as they are defined multiple times in the json).
     */
    UPROPERTY(VisibleAnywhere, Category = "ObjectDefinitions")
    mutable TSet<FName> FeatureTypes;

    UPROPERTY(VisibleAnywhere, Category = "ObjectDefinitions")
    TMap<FName, FArticyTemplateFeatureDef> FeatureDefs;
};
