//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ObjectDefinitionsImport.h"
#include "Misc/App.h"
#include "ArticyEditorModule.h"
#include "ArticyImportData.h"
#include "CodeGeneration/CodeFileGenerator.h"
#include "ArticyBuiltinTypes.h"
#include "PredefinedTypes.h"
#include "ArticyFlowClasses.h"
#include "ArticyScriptFragment.h"
#include "ArticyEntity.h"
#include "JsonObjectConverter.h"
#include "UObject/ConstructorHelpers.h"

//---------------------------------------------------------------------------//

/**
 * Import template definition data from a JSON object.
 *
 * @param JsonObject A shared pointer to the JSON object containing the template definition.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateDef::ImportFromJson(const TSharedPtr<FJsonObject> JsonObject, const UArticyImportData* Data)
{
    if (!JsonObject.IsValid())
        return;

    JSON_TRY_STRING(JsonObject, TechnicalName);
    JSON_TRY_STRING(JsonObject, DisplayName);

    JSON_TRY_ARRAY(JsonObject, Features,
        {
            FArticyTemplateFeatureDef def;
            def.ImportFromJson(item->AsObject(), Data);
            Features.Add(def);
            ArticyType.Features.Add(def.GetDisplayName());
        });

    ArticyType.HasTemplate = true;
    ArticyType.TechnicalName = TechnicalName;
    ArticyType.LocaKey_DisplayName = DisplayName;
}

/**
 * Generates feature definitions in the header file using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateDef::GenerateFeaturesDefs(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    for (const auto& feat : Features)
        feat.GenerateDefCode(header, Data);
}

/**
 * Generates property definitions in the header file using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateDef::GenerateProperties(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    for (const auto& feat : Features)
        feat.GeneratePropertyCode(header, Data);
}

/**
 * Gather scripts from JSON values and adds them to the ArticyImportData.
 *
 * @param Values A shared pointer to the JSON object containing the values.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateDef::GatherScripts(const TSharedPtr<FJsonObject> Values, UArticyImportData* Data) const
{
    for (const auto& feat : Features)
    {
        static const TSharedPtr<FJsonObject>* featureJson;
        if (Values->TryGetObjectField(feat.GetTechnicalName(), featureJson))
            feat.GatherScripts(*featureJson, Data);
    }
}

/**
 * Initializes a model with data from JSON values.
 *
 * @param Model A pointer to the UArticyPrimitive object.
 * @param Path The path of the model.
 * @param Values A shared pointer to the JSON object containing the values.
 * @param Data A pointer to the UArticyImportData object.
 * @param PackageName The name of the package.
 */
void FArticyTemplateDef::InitializeModel(UArticyPrimitive* Model, const FString& Path, const TSharedPtr<FJsonObject> Values, const UArticyImportData* Data, const FString& PackageName) const
{
    for (const auto& feat : Features)
    {
        static const TSharedPtr<FJsonObject>* featureJson;
        if (Values->TryGetObjectField(feat.GetTechnicalName(), featureJson))
            feat.InitializeModel(Model, Path, *featureJson, Data, PackageName);
    }

    Model->ArticyType.MergeChild(ArticyType);
}

//---------------------------------------------------------------------------//

/**
 * Imports object definition data from a JSON object.
 *
 * @param JsonObjDef A shared pointer to the JSON object containing the object definition.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyObjectDef::ImportFromJson(const TSharedPtr<FJsonObject> JsonObjDef, const UArticyImportData* Data)
{
    if (!JsonObjDef.IsValid())
        return;

    JSON_TRY_FNAME(JsonObjDef, Type);
    JSON_TRY_FNAME(JsonObjDef, Class);
    JSON_TRY_FNAME(JsonObjDef, InheritsFrom);

    DefType = EObjectDefType::Model;
    JSON_TRY_ARRAY(JsonObjDef, Properties,
        {
            FArticyPropertyDef prop;
            prop.ImportFromJson(item->AsObject(), Data);
            Properties.Add(prop);

            FArticyPropertyInfo info;
            FString name = prop.GetPropetyName().ToString();
            info.LocaKey_DisplayName = name;
            ArticyType.Properties.Add(info);
        });

    JSON_TRY_OBJECT(JsonObjDef, Values,
        {
            DefType = EObjectDefType::Enum;
            ArticyType.IsEnum = true;
            for (const auto& json : (*obj)->Values)
            {
                FArticyEnumValue val;
                val.ImportFromJson(json);
                Values.Add(val);

                FArticyEnumValueInfo info;
                info.LocaKey_DisplayName = val.Name;
                info.Value = val.Value;
                ArticyType.EnumValues.Add(info);
            }
        });

    JSON_TRY_OBJECT(JsonObjDef, Template,
        {
            DefType = EObjectDefType::Template;
            Template.ImportFromJson(*obj, Data);
            ArticyType.HasTemplate = true;
            ArticyType.MergeParent(Template.ArticyType);
        });

    ArticyType.CPPType = GetCppType(Data, false);
}

/**
 * Checks if a property is already defined in the base class.
 *
 * @param Property The name of the property.
 * @param Data A pointer to the UArticyImportData object.
 * @return True if the property is a base property, false otherwise.
 */
bool FArticyObjectDef::IsBaseProperty(FName Property, const UArticyImportData* Data) const
{
    const auto& baseClass = FArticyObjectDefinitions::GetDefaultBaseClass(Class, Data);
    return IArticyReflectable::HasProperty(baseClass.StaticClass, Property);
}

/**
 * Generates code for the object definition using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyObjectDef::GenerateCode(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    if (FArticyPredefTypes::IsPredefinedType(GetOriginalType()))
    {
        UE_LOG(LogArticyEditor, Log, TEXT("Skipped import of %s as it is a predefined type (%s)."), *GetOriginalType().ToString(), *Data->GetObjectDefs().GetCppType(GetOriginalType(), Data, false));
        return;
    }

    header.Line();
    header.Comment("--------------------------------------------------------------------------------");
    header.Line();

    if (DefType == EObjectDefType::Enum)
    {
        header.Enum(GetCppType(Data, false), "UENUM generated form ArticyObjectDef " + Type.ToString(), true, Values);
    }
    else
    {
        if (DefType == EObjectDefType::Template)
        {
            //generate the template features
            Template.GenerateFeaturesDefs(header, Data);
        }

        header.Class(GetCppType(Data, false) + " : public " + GetCppBaseClasses(Data), "UCLASS generated from ArticyObjectDef " + Type.ToString(), true, [&]
            {
                header.Line("public:", false, true, -1);
                header.Line();

                //implement feature interfaces
                for (const auto& feature : Template.GetFeatures())
                {
                    header.Method(feature.GetCppType(Data, true), "GetFeature" + feature.GetTechnicalName(), "", [&]
                        {
                            header.Line("return " + feature.GetTechnicalName(), true);
                        }, CodeGenerator::GetFeatureInterfaceClassName(Data, feature) + " implementation", false, "", "const override");
                }

                //declare all properties
                for (const auto& prop : Properties)
                {
                    //check if this property is already defined in the base class
                    if (IsBaseProperty(prop.GetPropetyName(), Data))
                        continue;

                    prop.GenerateCode(header, Data);
                }

                if (DefType == EObjectDefType::Template)
                    Template.GenerateProperties(header, Data);
            });
    }
}

/**
 * Gather scripts from model definition and adds them to the UArticyImportData.
 *
 * @param Vals The model definition containing the values.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyObjectDef::GatherScripts(const FArticyModelDef& Vals, UArticyImportData* Data) const
{
    if (DefType == EObjectDefType::Enum)
        return; //there are no scripts hidden in enums ;)

    if (Class != Type)
    {
        //gather scripts in parent class
        auto parentDef = Data->GetObjectDefs().GetTypes().Find(Class);
        if (parentDef)
            parentDef->GatherScripts(Vals, Data);
    }

    //gather script properties
    auto propertiesJson = Vals.GetPropertiesJson();
    for (const auto& prop : Properties)
        prop.GatherScript(propertiesJson, Data);

    //gather scripts in template
    auto featuresJson = Vals.GetTemplatesJson();
    if (featuresJson.IsValid())
        Template.GatherScripts(featuresJson, Data);
}

/**
 * Initializes a model with data from the model definition.
 *
 * @param Model A pointer to the UArticyPrimitive object.
 * @param Vals The model definition containing the values.
 * @param Data A pointer to the UArticyImportData object.
 * @param PackageName The name of the package.
 */
void FArticyObjectDef::InitializeModel(
    UArticyPrimitive* Model,
    const FArticyModelDef& Vals,
    const UArticyImportData* Data,
    const FString& PackageName) const
{
    if (DefType == EObjectDefType::Enum)
    {
        ensure(false);
        UE_LOG(LogArticyEditor, Error, TEXT("Cannot initialize type %s, as it is an enum type!"), *Type.ToString());
        return;
    }

    if (Class != Type)
    {
        //initialize parent-class data first
        auto parentDef = Data->GetObjectDefs().GetTypes().Find(Class);
        if (parentDef)
            parentDef->InitializeModel(Model, Vals, Data, PackageName);
    }

    {
        //try setting the "meta" data (not stored in the properties array)
        const auto& AssetRef = FName{ TEXT("AssetRef") };
        const auto& Category = FName{ TEXT("Category") };
        Model->SetProp(AssetRef, Vals.GetAssetRef());
        Model->SetProp(Category, Vals.GetAssetCat());
    }

    const auto& nameAndId = Vals.GetNameAndId();

    //set all properties
    auto propertiesJson = Vals.GetPropertiesJson();
    //then set the rest of the properties
    for (const auto& prop : Properties)
        prop.InitializeModel(Model, nameAndId, propertiesJson, Data, PackageName);

    //set the features (if this is a template)
    auto featuresJson = Vals.GetTemplatesJson();
    if (featuresJson.IsValid())
        Template.InitializeModel(Model, nameAndId, featuresJson, Data, PackageName);
    else
        ensure(Template.GetDisplayName().IsEmpty());

    Model->ArticyType.MergeChild(ArticyType);
}

/**
 * Returns the C++ type of the object definition.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @param bForProperty Whether the type is for a property.
 * @return The C++ type as a string.
 */
FString FArticyObjectDef::GetCppType(const UArticyImportData* Data, const bool bForProperty) const
{
    switch (DefType)
    {
    case EObjectDefType::Enum:
        return FString::Printf(TEXT("E%s%s"), *Data->GetProject().TechnicalName, *Type.ToString());
    case EObjectDefType::Model:
    case EObjectDefType::Template:
        return FString::Printf(TEXT("U%s%s%s"), *Data->GetProject().TechnicalName, *Type.ToString(), bForProperty ? TEXT("*") : TEXT(""));

    default:
        return "UNKNOWN_DEFTYPE";
    }
}

/**
 * Returns the C++ base classes of the object definition.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @return The C++ base classes as a string.
 */
FString FArticyObjectDef::GetCppBaseClasses(const UArticyImportData* Data) const
{
    FString baseClasses;

    if (InheritsFrom.IsNone())
        baseClasses = FArticyObjectDefinitions::GetDefaultBaseClass(Class, Data).CppTypeName;
    else
        baseClasses = Data->GetObjectDefs().GetCppType(InheritsFrom, Data, false);

    //collect all ObjectWith<..> interfaces
    TSet<FName> interfaces;
    for (const auto& prop : Properties)
    {
        if (IsBaseProperty(prop.GetPropetyName(), Data))
            continue;

        auto i = FArticyObjectDefinitions::GetProviderInterface(prop);
        if (!i.IsNone() && !interfaces.Contains(i))
        {
            interfaces.Add(i);
            baseClasses += ",\n public " + i.ToString();
        }
    }
    for (const auto& feature : Template.GetFeatures())
    {
        baseClasses += ",\n public " + CodeGenerator::GetFeatureInterfaceClassName(Data, feature, false);
    }

    return baseClasses;
}

/**
 * Returns the features of the object definition.
 *
 * @return A constant reference to the array of features.
 */
const TArray<FArticyTemplateFeatureDef>& FArticyObjectDef::GetFeatures() const
{
    if (DefType == EObjectDefType::Template)
        return Template.GetFeatures();

    static const TArray<FArticyTemplateFeatureDef> Empty;
    return Empty;
}

//---------------------------------------------------------------------------//

/**
 * Import property definition data from a JSON object.
 *
 * @param JsonProperty A shared pointer to the JSON object containing the property definition.
 * @param Data A pointer to the UArticyImportData object.
 * @param OptionalConstraints An optional array of template constraints.
 */
void FArticyPropertyDef::ImportFromJson(const TSharedPtr<FJsonObject> JsonProperty, const UArticyImportData* Data, const TArray<FArticyTemplateConstraint>* OptionalConstraints)
{
    if (!JsonProperty.IsValid())
        return;

    JSON_TRY_FNAME(JsonProperty, Property);

    JSON_TRY_FNAME(JsonProperty, Type);
    //check if this needs to be localized
    //if(Data->GetSettings().set_Localization) //the setting is ignored in the UE plugin
    {
        bool bIsLocalized = false;
        const static auto string = FName{ TEXT("string") }; //for now, localization is only possible for strings!

        //if this is part of a template, the constraint tells if the property should be localized
        if (OptionalConstraints)
        {
            const FArticyTemplateConstraint* constraint = OptionalConstraints->FindByPredicate([this](const FArticyTemplateConstraint& Item)
                {
                    return Item.Property == Property.ToString();
                });

            if (ensure(constraint)) //the constraint should exist
                bIsLocalized = constraint->IsLocalized && Type == string;
        }
        else if (Type == string)
        {
            //this properties are always localized
            static TArray<FName> LocalizedProperties;
            if (LocalizedProperties.Num() == 0)
            {
                LocalizedProperties.Add(TEXT("StageDirections"));
                LocalizedProperties.Add(TEXT("DisplayName"));
                LocalizedProperties.Add(TEXT("MenuText"));
                LocalizedProperties.Add(TEXT("Text"));
            }

            //check if the property is one of the LocalizedProperties
            bIsLocalized = LocalizedProperties.Contains(Property);
        }

        //localized strings get the FText type instead
        if (bIsLocalized)
        {
            static const auto& ftext = FName{ TEXT("FText") };
            Type = ftext;
        }
    }

    JSON_TRY_FNAME(JsonProperty, ItemType);

    JSON_TRY_STRING(JsonProperty, DisplayName);
    if (DisplayName == "")
        DisplayName = Property.ToString();

    JSON_TRY_STRING(JsonProperty, Tooltip);

    ArticyType.LocaKey_DisplayName = DisplayName;
    ArticyType.CPPType = GetCppType(Data);
}

/**
 * Generates code for the property definition using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyPropertyDef::GenerateCode(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    //generate a variable for each Property
    header.Variable(GetCppType(Data), Property.ToString(), FArticyObjectDefinitions::GetCppDefaultValue(Type), "", true,
        FString::Printf(TEXT("EditAnywhere, BlueprintReadWrite, meta=(DisplayName=\"%s\")"), *DisplayName));
}

/**
 * Gathers script fragments from JSON and adds them to the UArticyImportData.
 *
 * @param JsonObject A shared pointer to the JSON object containing the script.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyPropertyDef::GatherScript(const TSharedPtr<FJsonObject>& JsonObject, UArticyImportData* Data) const
{
    if (!JsonObject.IsValid())
        return;

    if (!ItemType.IsNone())
    {
        //check if it's an array of pins

        //this is a bit hacky, but at this time we don't know much about the properties in the array,
        //so we can't just call GatherScript on that properties
        static const auto& InputPin = FName{ "inputpin" };
        static const auto& OutputPin = FName{ "outputpin" };
        const auto& isOutputPin = ItemType == OutputPin;
        if (isOutputPin || ItemType == InputPin)
        {
            //array may not be contained in values
            const TArray<TSharedPtr<FJsonValue>>* pins;
            if (JsonObject->TryGetArrayField(Property.ToString(), pins) && pins)
            {
                for (auto pin : *pins)
                {
                    if (!pin.IsValid() || !ensure(pin->Type == EJson::Object))
                        continue;

                    FString value;
                    //property may not be contained in values
                    if (pin->AsObject()->TryGetStringField(TEXT("Text"), value))
                        Data->AddScriptFragment(value, isOutputPin);
                }
            }
        }
    }
    else
    {
        //check if it's a script_condition or a script_instruction

        //not case sensitive!!
        static const auto& ScriptCondition = FName{ "script_condition" };
        static const auto& ScriptInstruction = FName{ "script_instruction" };

        const auto& type = GetOriginalType();
        const auto& isInstruction = type == ScriptInstruction;
        if (isInstruction || type == ScriptCondition)
        {
            //property may not be contained in values
            FString value;
            if (JsonObject->TryGetStringField(Property.ToString(), value))
                Data->AddScriptFragment(value, isInstruction);
        }
    }
}

/**
 * Initializes a model with data from JSON.
 *
 * @param Model A pointer to the UArticyBaseObject.
 * @param Path The path of the model.
 * @param JsonObject A shared pointer to the JSON object containing the data.
 * @param Data A pointer to the UArticyImportData object.
 * @param PackageName The name of the package.
 */
void FArticyPropertyDef::InitializeModel(
    UArticyBaseObject* Model,
    const FString& Path,
    const TSharedPtr<FJsonObject>& JsonObject,
    const UArticyImportData* Data,
    const FString& PackageName) const
{
    auto jsonValue = JsonObject.IsValid() ? JsonObject->TryGetField(Property.ToString()) : nullptr;

    //property may not be contained in values
    if (!jsonValue.IsValid() || jsonValue->IsNull())
        return;

    FArticyObjectDefinitions::SetProp(ItemType.IsNone() ? Type : ItemType, GetPropetyName(), Model, Path + "." + Property.ToString(), jsonValue, PackageName);

    Model->ArticyType.MergeParent(ArticyType);
}

/**
 * Returns the C++ type of the property definition.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @return The C++ type as a string.
 */
FString FArticyPropertyDef::GetCppType(const UArticyImportData* Data) const
{
    auto type = Data->GetObjectDefs().GetCppType(Type, Data, true);

    //if there's a placeholder, fill in the ItemType
    if (type.Contains(TEXT("?")))
        type = type.Replace(TEXT("?"), *Data->GetObjectDefs().GetCppType(ItemType, Data, true));

    return type;
}

//---------------------------------------------------------------------------//

/**
 * Imports an array of FString from JSON values.
 *
 * @param Json A pointer to the array of JSON values.
 * @param OutArray A reference to the array to store the FString values.
 */
void ImportFStringArray(const TArray<TSharedPtr<FJsonValue>>* Json, TArray<FString>& OutArray)
{
    OutArray.Reset(Json ? Json->Num() : 0);

    if (!Json)
        return;

    for (const auto& type : *Json)
        OutArray.Add(type->AsString());
}

/**
 * Import template constraint data from a JSON object.
 *
 * @param JsonProperty A shared pointer to the JSON object containing the constraint.
 */
void FArticyTemplateConstraint::ImportFromJson(const TSharedPtr<FJsonObject> JsonProperty)
{
    if (!JsonProperty.IsValid())
        return;

    JSON_TRY_STRING(JsonProperty, Property);
    JSON_TRY_STRING(JsonProperty, Type);
    JSON_TRY_BOOL(JsonProperty, IsLocalized);

    //the rest of the 
}

/**
 * Imports enum value data from a JSON key-value pair.
 *
 * @param JsonKeyValue A key-value pair containing the enum value data.
 */
void FArticyEnumValue::ImportFromJson(const TPair<FString, TSharedPtr<FJsonValue>> JsonKeyValue)
{
    if (!JsonKeyValue.Value.IsValid())
        return;

    Name = JsonKeyValue.Key;

    uint32 val;
    JsonKeyValue.Value->TryGetNumber(val);
    Value = val;
}

/**
 * Import template feature definition data from a JSON object.
 *
 * @param JsonObject A shared pointer to the JSON object containing the feature definition.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateFeatureDef::ImportFromJson(const TSharedPtr<FJsonObject> JsonObject, const UArticyImportData* Data)
{
    if (!JsonObject.IsValid())
        return;

    JSON_TRY_STRING(JsonObject, TechnicalName);
    JSON_TRY_STRING(JsonObject, DisplayName);

    JSON_TRY_ARRAY(JsonObject, Constraints,
        {
            FArticyTemplateConstraint con;
            con.ImportFromJson(item->AsObject());
            Constraints.Add(con);
        });

    JSON_TRY_ARRAY(JsonObject, Properties,
        {
            FArticyPropertyDef prop;
            prop.ImportFromJson(item->AsObject(), Data, &Constraints);
            Properties.Add(prop);

            FArticyPropertyInfo info;
            FString name = prop.GetPropetyName().ToString();
            info.LocaKey_DisplayName = name;
            ArticyType.Properties.Add(info);
        });

    ArticyType.TechnicalName = TechnicalName;
    ArticyType.LocaKey_DisplayName = DisplayName;
    ArticyType.CPPType = GetCppType(Data, false);
}

/**
 * Generates definition code for the template feature using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateFeatureDef::GenerateDefCode(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    if (!Data->GetObjectDefs().IsNewFeatureType(*GetCppType(Data, false)))
        return;

    //generate feature type
    header.Class(GetCppType(Data, false) + " : public UArticyBaseFeature", "UCLASS generated from Articy " + DisplayName + " Feature", true, [&]
        {
            header.Line("public:", false, true, -1);
            header.Line();

            for (const auto& prop : Properties)
                prop.GenerateCode(header, Data);

            //NOTE Constraints are not part of this implementation
            /*for(const auto con : Constraints) .. */
        });
}

/**
 * Generates property code for the template feature using CodeFileGenerator.
 *
 * @param header A reference to the CodeFileGenerator.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateFeatureDef::GeneratePropertyCode(CodeFileGenerator& header, const UArticyImportData* Data) const
{
    header.Variable(GetCppType(Data, true), TechnicalName, "", DisplayName, true, FString::Printf(TEXT("VisibleAnywhere, BlueprintReadOnly")));
}

/**
 * Gather scripts from JSON and adds them to the UArticyImportData.
 *
 * @param Json A shared pointer to the JSON object containing the scripts.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyTemplateFeatureDef::GatherScripts(const TSharedPtr<FJsonObject>& Json, UArticyImportData* Data) const
{
    for (const auto& prop : Properties)
        prop.GatherScript(Json, Data);
}

/**
 * Initializes a model with data from JSON.
 *
 * @param Model A pointer to the UArticyPrimitive object.
 * @param Path The path of the model.
 * @param Json A shared pointer to the JSON object containing the data.
 * @param Data A pointer to the UArticyImportData object.
 * @param PackageName The name of the package.
 */
void FArticyTemplateFeatureDef::InitializeModel(
    UArticyPrimitive* Model,
    const FString& Path,
    const TSharedPtr<FJsonObject>& Json,
    const UArticyImportData* Data,
    const FString& PackageName) const
{
    //create new instance of the feature
    auto feature = NewObject<UArticyBaseFeature>(Model, GetUClass(Data));
    Model->SetProp(*TechnicalName, feature);

    const auto& path = Path + "." + *TechnicalName;
    for (const auto& prop : Properties)
        prop.InitializeModel(feature, path, Json, Data, PackageName);

    Model->ArticyType.MergeChild(ArticyType);
}

/**
 * Returns the UClass of the template feature.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @return A pointer to the UClass of the feature.
 */
UClass* FArticyTemplateFeatureDef::GetUClass(const UArticyImportData* Data) const
{
    auto className = GetCppType(Data, false);
    //remove the 'U' at the beginning of the class name, since FindOrLoadClass doesn't like that one....
    className.RemoveAt(0);

    auto fullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), FApp::GetProjectName(), *className);
    return ConstructorHelpersInternal::FindOrLoadClass(fullClassName, UObject::StaticClass());
}

/**
 * Returns the C++ type of the template feature.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @param bAsVariable Whether the type is for a variable.
 * @return The C++ type as a string.
 */
FString FArticyTemplateFeatureDef::GetCppType(const UArticyImportData* Data, bool bAsVariable) const
{
    return FString::Printf(TEXT("U%s%sFeature%s"), *Data->GetProject().TechnicalName, *TechnicalName, bAsVariable ? TEXT("*") : TEXT(""));
}

//---------------------------------------------------------------------------//

/**
 * Imports object definitions from a JSON array.
 *
 * @param Json A pointer to the array of JSON values containing the object definitions.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyObjectDefinitions::ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json, const UArticyImportData* Data)
{
    Types.Reset();
    FeatureTypes.Reset();
    FeatureDefs.Reset();

    if (!Json)
        return;

    for (const auto& type : *Json)
    {
        const auto& obj = type->AsObject();
        if (!obj.IsValid())
            continue;

        FArticyObjectDef def;
        def.ImportFromJson(obj, Data);
        Types.Add(def.GetOriginalType(), def);

        for (auto& feature : def.GetFeatures())
        {
            FName key = FName(*feature.GetTechnicalName());
            if (!FeatureDefs.Contains(key))
                FeatureDefs.Add(key, feature);
        }
    }
}

/**
 * Gather text definitions from a JSON object.
 *
 * @param Json A shared pointer to the JSON object containing the text definitions.
 */
void FArticyObjectDefinitions::GatherText(const TSharedPtr<FJsonObject>& Json)
{
    for (auto JsonValue = Json->Values.CreateConstIterator(); JsonValue; ++JsonValue)
    {
        const FString Name = (*JsonValue).Key;
        const TSharedPtr<FJsonValue> Value = (*JsonValue).Value;

        FArticyTexts Text;
        Text.ImportFromJson(Value->AsObject());
        Texts.Add(Name, Text);
    }
}

/**
 * Gather scripts from model definition and adds them to the UArticyImportData.
 *
 * @param Values The model definition containing the values.
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyObjectDefinitions::GatherScripts(const FArticyModelDef& Values, UArticyImportData* Data) const
{
    const auto& def = Types.Find(Values.GetType());
    if (ensure(def))
        def->GatherScripts(Values, Data);
    else
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Model type %s for Model %s not found in definitions!"), *Values.GetType().ToString(), *Values.GetTechnicalName());
    }
}

/**
 * Initializes a model with data from the model definition.
 *
 * @param Model A pointer to the UArticyPrimitive object.
 * @param Values The model definition containing the values.
 * @param Data A pointer to the UArticyImportData object.
 * @param PackageName The name of the package.
 */
void FArticyObjectDefinitions::InitializeModel(
    UArticyPrimitive* Model,
    const FArticyModelDef& Values,
    const UArticyImportData* Data,
    const FString& PackageName) const
{
    auto def = Types.Find(Values.GetType());
    if (ensure(def))
        def->InitializeModel(Model, Values, Data, PackageName);
    else
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Model type %s for Model %s not found in definitions!"), *Values.GetType().ToString(), *Values.GetTechnicalName());
    }
}

/**
 * Returns the C++ type of an object definition.
 *
 * @param OriginalType The original type of the object definition.
 * @param Data A pointer to the UArticyImportData object.
 * @param bForProperty Whether the type is for a property.
 * @return The C++ type as a string.
 */
FString FArticyObjectDefinitions::GetCppType(const FName& OriginalType, const UArticyImportData* Data, const bool bForProperty) const
{
    //first look up in PredefinedTypes
    auto predefType = FArticyPredefTypes::Get().Find(OriginalType);
    if (predefType && *predefType)
        return bForProperty ? (*predefType)->CppPropertyType : (*predefType)->CppType;

    //not a predefined type, look up in imported Types
    auto type = Types.Find(OriginalType);
    if (ensureMsgf(type, TEXT("Type %s was not found in PredefinedTypes or imported Types!"), *OriginalType.ToString()))
        return type->GetCppType(Data, bForProperty);

    return FString::Printf(TEXT("%s_NOT_FOUND"), *OriginalType.ToString());
}

/**
 * Returns the default base class for an object definition.
 *
 * @param OriginalType The original type of the object definition.
 * @param Data A pointer to the UArticyImportData object.
 * @return A reference to the FClassInfo struct containing the base class information.
 */
const FArticyObjectDefinitions::FClassInfo& FArticyObjectDefinitions::GetDefaultBaseClass(const FName& OriginalType, const UArticyImportData* Data)
{
    static TMap<FName, FClassInfo> DefaultBaseClasses;
    if (DefaultBaseClasses.Num() == 0)
    {
        DefaultBaseClasses.Add("Asset", FClassInfo{ "UArticyAsset", UArticyAsset::StaticClass() });
        DefaultBaseClasses.Add("Condition", FClassInfo{ "UArticyCondition", UArticyCondition::StaticClass() });
        DefaultBaseClasses.Add("Comment", FClassInfo{ "UArticyComment", UArticyComment::StaticClass() });
        DefaultBaseClasses.Add("DialogueFragment", FClassInfo{ "UArticyDialogueFragment", UArticyDialogueFragment::StaticClass() });
        DefaultBaseClasses.Add("Dialogue", FClassInfo{ "UArticyDialogue", UArticyDialogue::StaticClass() });
        DefaultBaseClasses.Add("Document", FClassInfo{ "UArticyDocument", UArticyDocument::StaticClass() });
        DefaultBaseClasses.Add("Entity", FClassInfo{ "UArticyEntity", UArticyEntity::StaticClass() });
        DefaultBaseClasses.Add("FlowFragment", FClassInfo{ "UArticyFlowFragment", UArticyFlowFragment::StaticClass() });
        DefaultBaseClasses.Add("Hub", FClassInfo{ "UArticyHub", UArticyHub::StaticClass() });
        DefaultBaseClasses.Add("LocationImage", FClassInfo{ "UArticyLocationImage", UArticyLocationImage::StaticClass() });
        DefaultBaseClasses.Add("LocationText", FClassInfo{ "UArticyLocationText", UArticyLocationText::StaticClass() });
        DefaultBaseClasses.Add("Instruction", FClassInfo{ "UArticyInstruction", UArticyInstruction::StaticClass() });
        DefaultBaseClasses.Add("Jump", FClassInfo{ "UArticyJump", UArticyJump::StaticClass() });
        DefaultBaseClasses.Add("Link", FClassInfo{ "UArticyLink", UArticyLink::StaticClass() });
        DefaultBaseClasses.Add("Location", FClassInfo{ "UArticyLocation", UArticyLocation::StaticClass() });
        DefaultBaseClasses.Add("Path", FClassInfo{ "UArticyPath", UArticyPath::StaticClass() });
        DefaultBaseClasses.Add("Spot", FClassInfo{ "UArticySpot", UArticySpot::StaticClass() });
        DefaultBaseClasses.Add("TextObject", FClassInfo{ "UArticyTextObject", UArticyTextObject::StaticClass() });
        DefaultBaseClasses.Add("UserFolder", FClassInfo{ "UArticyUserFolder", UArticyUserFolder::StaticClass() });
        DefaultBaseClasses.Add("Zone", FClassInfo{ "UArticyZone", UArticyZone::StaticClass() });
    }

    auto base = DefaultBaseClasses.Find(OriginalType);
    if (base)
        return *base;

    //not one of the FlowClasses, use UArticyObject as base class
    static const auto& ArticyObject = FClassInfo{ "UArticyObject", UArticyObject::StaticClass() };
    return ArticyObject;
}

/**
 * Returns the provider interface name for a given property.
 *
 * @param Property The property definition.
 * @return The provider interface name as an FName.
 */
const FName& FArticyObjectDefinitions::GetProviderInterface(const FArticyPropertyDef& Property)
{
    static TMap<FName, FName> ProviderInterfaces;
    if (ProviderInterfaces.Num() == 0)
    {
#define OBJECT_WITH_X(x) TEXT(x), TEXT("IArticyObjectWith" x)

        ProviderInterfaces.Add(OBJECT_WITH_X("Attachments"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Color"));
        ProviderInterfaces.Add(OBJECT_WITH_X("DisplayName"));
        ProviderInterfaces.Add(OBJECT_WITH_X("ExternalId"));
        ProviderInterfaces.Add(OBJECT_WITH_X("MenuText"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Position"));
        ProviderInterfaces.Add(OBJECT_WITH_X("PreviewImage"));
        ProviderInterfaces.Add(OBJECT_WITH_X("ShortId"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Size"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Speaker"));
        ProviderInterfaces.Add(OBJECT_WITH_X("StageDirections"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Target"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Text"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Transform"));
        ProviderInterfaces.Add(OBJECT_WITH_X("Vertices"));
        ProviderInterfaces.Add(OBJECT_WITH_X("ZIndex"));
    }

    auto i = ProviderInterfaces.Find(Property.GetPropetyName());
    if (i)
        return *i;

    static FName Empty;
    return Empty;
}

/**
 * Returns the default C++ value for a given type.
 *
 * @param OriginalType The original type.
 * @return The default value as a string.
 */
const FString& FArticyObjectDefinitions::GetCppDefaultValue(const FName& OriginalType)
{
    //first look up in PredefinedTypes
    auto predefType = FArticyPredefTypes::Get().Find(OriginalType);
    if (predefType && *predefType)
        return (*predefType)->CppDefaultValue;

    static const FString Empty = "";
    return Empty;
}

/**
 * Sets a property value on a model.
 *
 * @param OriginalType The original type of the property.
 * @param Property The name of the property.
 * @param PROP_SETTER_PARAMS Parameters for setting the property.
 */
void FArticyObjectDefinitions::SetProp(const FName& OriginalType, const FName& Property, PROP_SETTER_PARAMS)
{
    auto typePtr = FArticyPredefTypes::Get().Find(OriginalType);
    //if it's not a predefined type, it must be an enum - or an error ;)
    auto type = typePtr ? *typePtr : FArticyPredefTypes::GetEnum();

    if (ensure(type))
    {
        static const TArray<TSharedPtr<FJsonValue>>* jArray;
        if (Json->TryGetArray(jArray))
            type->SetArray(Property, Model, Path, *jArray, PackageName);
        else
            type->SetProp(Property, PROP_SETTER_ARGS);
    }
}

/**
 * Checks if a feature type is new.
 *
 * @param CppType The C++ type of the feature.
 * @return True if the feature type is new, false otherwise.
 */
bool FArticyObjectDefinitions::IsNewFeatureType(const FName& CppType) const
{
    bool bAlreadyPresent;
    FeatureTypes.Add(CppType, &bAlreadyPresent);

    return !bAlreadyPresent;
}
