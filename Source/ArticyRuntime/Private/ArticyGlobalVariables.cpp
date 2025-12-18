//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyGlobalVariables.h"
#include "Kismet/KismetStringLibrary.h"
#include "Engine/GameInstance.h"
#include "ArticyRuntimeModule.h"
#include "ArticyPluginSettings.h"
#include "ArticyFlowPlayer.h"
#include "ArticyAlternativeGlobalVariables.h"
#include "AssetRegistry/AssetData.h"
#include <ArticyPins.h>

/**
 * Constructor for FArticyGvName using full variable name.
 * @param FullVariableName The full name of the variable, including namespace.
 */
FArticyGvName::FArticyGvName(const FName FullVariableName)
{
    SetByFullName(FullVariableName);
}

/**
 * Constructor for FArticyGvName using separate namespace and variable names.
 * @param VariableNamespace The namespace of the variable.
 * @param VariableName The name of the variable.
 */
FArticyGvName::FArticyGvName(const FName VariableNamespace, const FName VariableName)
{
    SetByNamespaceAndVariable(VariableNamespace, VariableName);
}

/**
 * Sets the variable name and namespace using the full name.
 * @param FullVariableName The full name of the variable, including namespace.
 */
void FArticyGvName::SetByFullName(const FName FullVariableName)
{
    FString variableString;
    FString namespaceString;
    if (FullVariableName.ToString().Split(TEXT("."), &namespaceString, &variableString))
    {
        FullName = FullVariableName;
        Namespace = FName(*namespaceString);
        Variable = FName(*variableString);
    }
}

/**
 * Sets the variable name and namespace using separate names.
 * @param VariableNamespace The namespace of the variable.
 * @param VariableName The name of the variable.
 */
void FArticyGvName::SetByNamespaceAndVariable(const FName VariableNamespace, const FName VariableName)
{
    if (!VariableNamespace.IsNone() && !VariableName.IsNone())
    {
        Namespace = VariableNamespace;
        Variable = VariableName;
        FullName = FName(*FString::Printf(TEXT("%s.%s"), *Namespace.ToString(), *Variable.ToString()));
    }
}

/**
 * Gets the namespace of the variable. Sets the namespace if not already set.
 * @return The namespace of the variable.
 */
const FName& FArticyGvName::GetNamespace()
{
    if (!Namespace.IsNone())
        return Namespace;

    SetByFullName(FullName);
    return Namespace;
}

/**
 * Gets the variable name. Sets the variable name if not already set.
 * @return The name of the variable.
 */
const FName& FArticyGvName::GetVariable()
{
    if (!Variable.IsNone())
        return Variable;

    SetByFullName(FullName);
    return Variable;
}

/**
 * Gets the full name of the variable. Sets the full name if not already set.
 * @return The full name of the variable.
 */
const FName& FArticyGvName::GetFullName()
{
    if (!FullName.IsNone())
        return FullName;

    SetByNamespaceAndVariable(Namespace, Variable);
    return FullName;
}

//---------------------------------------------------------------------------//

/**
 * Gets the current shadow level of the variable store.
 * @return The shadow level of the variable store.
 */
uint32 UArticyVariable::GetStoreShadowLevel() const
{
    return Store->GetShadowLevel();
}

/**
 * Broadcasts a notification that a variable has changed.
 * @param Variable The variable that changed.
 */
void UArticyBaseVariableSet::BroadcastOnVariableChanged(UArticyVariable* Variable)
{
    OnVariableChanged.Broadcast(Variable);
}

//---------------------------------------------------------------------------//

/**
 * Retrieves the default global variables object, creating a clone if necessary.
 * @param WorldContext The context within which the object is retrieved.
 * @return A pointer to the default UArticyGlobalVariables object.
 */
UArticyGlobalVariables* UArticyGlobalVariables::GetDefault(const UObject* WorldContext)
{
    if (!Clone.IsValid())
    {
#if ENGINE_MAJOR_VERSION >= 5
        bool keepBetweenWorlds = UArticyPluginSettings::Get()->bKeepGlobalVariablesBetweenWorlds || WorldContext->GetWorld()->IsPartitionedWorld();
#else
        bool keepBetweenWorlds = UArticyPluginSettings::Get()->bKeepGlobalVariablesBetweenWorlds;
#endif

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetData;


#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
        AssetRegistryModule.Get().GetAssetsByClass(UArticyGlobalVariables::StaticClass()->GetClassPathName(), AssetData, true);
#else
        AssetRegistryModule.Get().GetAssetsByClass(UArticyGlobalVariables::StaticClass()->GetFName(), AssetData, true);
#endif        
        UArticyGlobalVariables* asset = nullptr;
        if (ensureMsgf(AssetData.Num() != 0, TEXT("ArticyGlobalVariables asset not found!")))
        {
            if (AssetData.Num() > 1)
                UE_LOG(LogTemp, Warning, TEXT("More than one ArticyGlobalVariables asset was found, this is not supported! The first one will be selected."));

            asset = Cast<UArticyGlobalVariables>(AssetData[0].GetAsset());
        }

        if (!asset)
            return nullptr;

        UE_LOG(LogArticyRuntime, Log, TEXT("Cloning GVs."));

        auto world = GEngine->GetWorldFromContextObjectChecked(WorldContext);
        ensureMsgf(world, TEXT("Getting world for GV cloning failed!"));

#if ENGINE_MAJOR_VERSION >= 5
        TObjectPtr<UArticyGlobalVariables> assetPtr = asset;
#else
        UArticyGlobalVariables* assetPtr = asset;
#endif

        if (keepBetweenWorlds)
        {
            Clone = DuplicateObject<UArticyGlobalVariables>(assetPtr, Cast<UObject>(world->GetGameInstance()), TEXT("Persistent Runtime GV"));
#if !WITH_EDITOR
            Clone->AddToRoot();
#endif
        }
        else
        {
            Clone = DuplicateObject<UArticyGlobalVariables>(assetPtr, Cast<UObject>(world), *FString::Printf(TEXT("%s GV"), *world->GetName()));
        }

        ensureMsgf(Clone.IsValid(), TEXT("Cloning GV asset failed!"));
    }

    return Clone.Get();
}

/**
 * Retrieves the mutable original global variables object.
 * @return A pointer to the original UArticyGlobalVariables object.
 */
UArticyGlobalVariables* UArticyGlobalVariables::GetMutableOriginal()
{
    static TWeakObjectPtr<UArticyGlobalVariables> Asset = nullptr;

    if (!Asset.IsValid())
    {
        //create a clone of the database
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
        AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetClassPathName(), AssetData, true);
#else
        AssetRegistryModule.Get().GetAssetsByClass(StaticClass()->GetFName(), AssetData, true);
#endif        
        if (AssetData.Num() != 0)
        {
            if (AssetData.Num() > 1)
            {
                UE_LOG(LogArticyRuntime, Warning, TEXT("More than one ArticyGV was found, this is not supported! The first one will be selected."));
            }

            Asset = Cast<UArticyGlobalVariables>(AssetData[0].GetAsset());
        }
        else
        {
            UE_LOG(LogArticyRuntime, Warning, TEXT("No ArticyDraftGV was found."));
        }
    }

    return Asset.Get();
}

/**
 * Retrieves a runtime clone of non-default global variables.
 * @param WorldContext The context within which the object is retrieved.
 * @param GVs Alternative global variables to clone.
 * @return A pointer to the cloned UArticyGlobalVariables object.
 */
UArticyGlobalVariables* UArticyGlobalVariables::GetRuntimeClone(const UObject* WorldContext, UArticyAlternativeGlobalVariables* GVs)
{
    // Special case: We're passed a nullptr. Use the default shared set.
    if (GVs == nullptr) { return GetDefault(WorldContext); }

    // Get unique name of GV set
    const FString Name = GVs->GetFullName();
    const FName Key = FName(*Name);

    // Check if we already have a clone
    const auto& Existing = OtherClones.Find(Key);
    if (Existing && Existing->IsValid())
    {
        // If so, return it
        return Existing->Get();
    }

    UE_LOG(LogTemp, Warning, TEXT("Cloning Override GVs %s"), *GVs->GetFullName());

    // Get world context
    auto world = GEngine->GetWorldFromContextObjectChecked(WorldContext);
    ensureMsgf(world, TEXT("Getting world for GV cloning failed!"));

    // Get global variable asset to clone
    UArticyGlobalVariables* asset = UArticyGlobalVariables::GetMutableOriginal();

    // Check if we're keeping global variable objects between worlds
    bool keepBetweenWorlds = UArticyPluginSettings::Get()->bKeepGlobalVariablesBetweenWorlds;

    // If so, duplicate and add to root
    UArticyGlobalVariables* NewClone = nullptr;

#if ENGINE_MAJOR_VERSION >= 5
    TObjectPtr<UArticyGlobalVariables> assetPtr = asset;
#else
    UArticyGlobalVariables* assetPtr = asset;
#endif        

    if (keepBetweenWorlds)
    {
        FString NewName = TEXT("Persistent Runtime GV Clone of ") + Name;
        NewClone = DuplicateObject<UArticyGlobalVariables>(assetPtr, Cast<UObject>(world->GetGameInstance()), *NewName);
#if !WITH_EDITOR
        NewClone->AddToRoot();
#endif
    }
    else
    {
        // Otherwise, add it to the active world
        NewClone = DuplicateObject(assetPtr, Cast<UObject>(world), *FString::Printf(TEXT("%s %s GV"), *world->GetName(), *Name));
    }

    // Store and return
    OtherClones.FindOrAdd(Key) = NewClone;
    return NewClone;
}

/**
 * Unloads the global variables, removing all changes.
 */
void UArticyGlobalVariables::UnloadGlobalVariables()
{
    if (Clone.IsValid())
    {
        Clone->RemoveFromRoot();
        Clone->ConditionalBeginDestroy();
        Clone = NULL;
    }
}

/**
 * Retrieves a namespace variable set by name.
 * @param Namespace The name of the namespace.
 * @return A pointer to the UArticyBaseVariableSet object.
 */
UArticyBaseVariableSet* UArticyGlobalVariables::GetNamespace(const FName Namespace)
{
    auto set = GetProp<UArticyBaseVariableSet*>(Namespace);
    if (!set)
    {
        if (bLogVariableAccess)
        {
            UE_LOG(LogArticyRuntime, Error, TEXT("GV Namespace %s not found!"), *Namespace.ToString());
        }
        return nullptr;
    }

    return set;
}

/**
 * Prints the value of a global variable.
 * @param GvName The name of the global variable.
 */
void UArticyGlobalVariables::PrintGlobalVariable(FArticyGvName GvName)
{
    bool bTmpLogVariableAccess = bLogVariableAccess;
    bool bPrintSuccessful = false;
    // turn off variable access logging for this function so that the result only prints once
    bLogVariableAccess = false;

    auto set = GetNamespace(GvName.GetNamespace());
    if (set)
    {
        UArticyVariable** basePtr = set->GetPropPtr<UArticyVariable*>(GvName.GetVariable());

        if (Cast<UArticyBool>(*basePtr))
        {
            bool boolSucceeded = false;
            auto boolValue = GetBoolVariable(GvName, boolSucceeded);
            UE_LOG(LogArticyRuntime, Display, TEXT("%s::%s = %s"), *GvName.GetNamespace().ToString(), *GvName.GetVariable().ToString(), boolValue ? *FString("True") : *FString("False"));
            bPrintSuccessful = true;
        }
        else if (Cast<UArticyInt>(*basePtr))
        {
            bool intSucceeded = false;
            auto intValue = GetIntVariable(GvName, intSucceeded);
            UE_LOG(LogArticyRuntime, Display, TEXT("%s::%s = %d"), *GvName.GetNamespace().ToString(), *GvName.GetVariable().ToString(), intValue);
            bPrintSuccessful = true;
        }
        else if (Cast<UArticyString>(*basePtr))
        {
            bool stringSucceeded = false;
            auto stringValue = GetStringVariable(GvName, stringSucceeded);
            UE_LOG(LogArticyRuntime, Display, TEXT("%s::%s = %s"), *GvName.GetNamespace().ToString(), *GvName.GetVariable().ToString(), *stringValue);
            bPrintSuccessful = true;
        }
    }

    if (!bPrintSuccessful)
    {
        UE_LOG(LogArticyRuntime, Error, TEXT("Unable to find variable: %s::%s"), *GvName.GetNamespace().ToString(), *GvName.GetVariable().ToString());
    }

    // restore prior logging mode
    bLogVariableAccess = bTmpLogVariableAccess;
}

/**
 * Retrieves a boolean variable's value by name.
 * @param GvName The name of the global variable.
 * @param bSucceeded Indicates whether the retrieval was successful.
 * @return The value of the boolean variable.
 */
const bool& UArticyGlobalVariables::GetBoolVariable(FArticyGvName GvName, bool& bSucceeded)
{
    return GetVariableValue<UArticyBool, bool>(GvName.GetNamespace(), GvName.GetVariable(), bSucceeded);
}

/**
 * Retrieves an integer variable's value by name.
 * @param GvName The name of the global variable.
 * @param bSucceeded Indicates whether the retrieval was successful.
 * @return The value of the integer variable.
 */
const int32& UArticyGlobalVariables::GetIntVariable(FArticyGvName GvName, bool& bSucceeded)
{
    return GetVariableValue<UArticyInt, int32>(GvName.GetNamespace(), GvName.GetVariable(), bSucceeded);
}

/**
 * Retrieves a string variable's value by name.
 * @param GvName The name of the global variable.
 * @param bSucceeded Indicates whether the retrieval was successful.
 * @return The value of the string variable.
 */
const FString& UArticyGlobalVariables::GetStringVariable(FArticyGvName GvName, bool& bSucceeded)
{
    return GetVariableValue<UArticyString, FString>(GvName.GetNamespace(), GvName.GetVariable(), bSucceeded);
}

/**
 * Sets a boolean variable's value by name.
 * @param GvName The name of the global variable.
 * @param Value The new value to set.
 */
void UArticyGlobalVariables::SetBoolVariable(FArticyGvName GvName, const bool Value)
{
    SetVariableValue<UArticyBool>(GvName.GetNamespace(), GvName.GetVariable(), Value);
}

/**
 * Sets an integer variable's value by name.
 * @param GvName The name of the global variable.
 * @param Value The new value to set.
 */
void UArticyGlobalVariables::SetIntVariable(FArticyGvName GvName, const int32 Value)
{
    SetVariableValue<UArticyInt>(GvName.GetNamespace(), GvName.GetVariable(), Value);
}

/**
 * Sets a string variable's value by name.
 * @param GvName The name of the global variable.
 * @param Value The new value to set.
 */
void UArticyGlobalVariables::SetStringVariable(FArticyGvName GvName, const FString Value)
{
    SetVariableValue<UArticyString>(GvName.GetNamespace(), GvName.GetVariable(), Value);
}

/**
 * Enables debug logging for global variable access.
 */
void UArticyGlobalVariables::EnableDebugLogging()
{
    bLogVariableAccess = true;
}

/**
 * Disables debug logging for global variable access.
 */
void UArticyGlobalVariables::DisableDebugLogging()
{
    bLogVariableAccess = false;
}

/**
 * Resets the visited state for all nodes.
 */
void UArticyGlobalVariables::ResetVisited()
{
    VisitedNodes.Reset();
    TMap<FArticyId, int> Empty;
    VisitedNodes.Add(Empty);
}

/**
 * Gets the seen counter for a specific object.
 * @param Object The object to query.
 * @return The seen counter value.
 */
int UArticyGlobalVariables::GetSeenCounter(const IArticyFlowObject* Object) const
{
    auto* Obj = Cast<UArticyPrimitive>(Object);
    if (Obj)
    {
        FArticyId targetId;
        targetId = Obj->GetId();

        if (VisitedNodes.IsEmpty())
            return 0;

        if (auto* counter = VisitedNodes.Top().Find(targetId))
        {
            return *counter;
        }
    }
    return 0;
}

/**
 * Sets the seen counter for a specific object.
 * @param Object The object to modify.
 * @param Value The new seen counter value.
 * @return The updated seen counter value.
 */
int UArticyGlobalVariables::SetSeenCounter(const IArticyFlowObject* Object, int Value)
{
    auto* Obj = Cast<UArticyPrimitive>(Object);
    if (Obj)
    {
        FArticyId targetId;
        targetId = Obj->GetId();

        if (VisitedNodes.IsEmpty())
        {
            TMap<FArticyId, int> Empty;
            VisitedNodes.Add(Empty);
        }

        // update if already tracked
        if (auto* counter = VisitedNodes.Top().Find(targetId))
        {
            return *counter = Value;
        }

        // add and return
        VisitedNodes.Top().Add(targetId, Value);
        return Value;
    }
    return 0;
}

/**
 * Increments the seen counter for a specific object.
 * @param Object The object to modify.
 * @return The updated seen counter value.
 */
int UArticyGlobalVariables::IncrementSeenCounter(const IArticyFlowObject* Object)
{
    auto* Obj = Cast<UArticyPrimitive>(Object);
    if (Obj)
    {
        FArticyId targetId;
        targetId = Obj->GetId();

        if (VisitedNodes.IsEmpty())
        {
            TMap<FArticyId, int> Empty;
            VisitedNodes.Add(Empty);
        }

        // update if already tracked
        if (auto* counter = VisitedNodes.Top().Find(targetId))
        {
            return ++(*counter);
        }
        // add and return
        VisitedNodes.Top().Add(targetId, 1);
        return 1;
    }
    return 0;
}

/**
 * Checks if a fallback evaluation is active for a specific object.
 * @param Object The object to query.
 * @return True if fallback evaluation is active, otherwise false.
 */
bool UArticyGlobalVariables::Fallback(const IArticyFlowObject* Object)
{
    auto* Obj = Cast<UArticyPrimitive>(Object);
    if (Obj)
    {
        FArticyId targetId;
        targetId = Obj->GetId();

        if (bIsFallbackEvaluation.IsEmpty())
            return false;

        if (auto* state = bIsFallbackEvaluation.Top().Find(targetId))
        {
            return *state;
        }
    }
    else
    {
        if (!bIsFallbackEvaluation.IsEmpty())
        {
            for (const auto& Elem : bIsFallbackEvaluation.Top())
            {
                if (Elem.Value)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * Sets the fallback evaluation state for a specific object.
 * @param Object The object to modify.
 * @param Value The new fallback evaluation state.
 */
void UArticyGlobalVariables::SetFallbackEvaluation(const IArticyFlowObject* Object, bool Value)
{
    auto* Obj = Cast<UArticyPrimitive>(Object);
    if (Obj)
    {
        FArticyId targetId;
        targetId = Obj->GetId();

        if (bIsFallbackEvaluation.IsEmpty())
        {
            TMap<FArticyId, bool> Empty;
            bIsFallbackEvaluation.Add(Empty);
        }

        // update if already tracked
        if (auto* state = bIsFallbackEvaluation.Top().Find(targetId))
        {
            *state = Value;
        }
        else
        {
            bIsFallbackEvaluation.Top().Add(targetId, Value);
        }
    }
    return;
}

/**
 * Pushes the current seen state onto the stack.
 */
void UArticyGlobalVariables::PushSeen()
{
    if (!VisitedNodes.IsEmpty())
    {
        TMap<FArticyId, int> Copy(VisitedNodes.Top());
        VisitedNodes.Push(Copy);
    }
    if (!bIsFallbackEvaluation.IsEmpty())
    {
        TMap<FArticyId, bool> Copy(bIsFallbackEvaluation.Top());
        bIsFallbackEvaluation.Push(Copy);
    }
}

/**
 * Pops the current seen state from the stack.
 */
void UArticyGlobalVariables::PopSeen()
{
    if (!bIsFallbackEvaluation.IsEmpty())
        bIsFallbackEvaluation.Pop();
    if (!VisitedNodes.IsEmpty())
        VisitedNodes.Pop();
}

TWeakObjectPtr<UArticyGlobalVariables> UArticyGlobalVariables::Clone;
TMap<FName, TWeakObjectPtr< UArticyGlobalVariables>> UArticyGlobalVariables::OtherClones;
