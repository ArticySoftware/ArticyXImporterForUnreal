//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArticyObject.h"
#include "ArticyFlowPlayer.h"
#include "ArticyFunctionLibrary.generated.h"

/**
 * Function library for handling Articy objects and IDs in Unreal Engine.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Converts an ArticyRef to an object.
     * @param Ref The ArticyRef to convert.
     * @param CastTo The subclass type to cast the object to.
     * @param WorldContext The context within which the object is retrieved.
     * @return The UArticyObject referenced by the ArticyRef.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Object", DefaultToSelf = "WorldContext", DeterminesOutputType = "CastTo"), Category = "ArticyRef")
    static UArticyObject* ArticyRef_GetObject(FArticyRef Ref, TSubclassOf<class UArticyObject> CastTo, const UObject* WorldContext);

    /**
     * Sets the referenced object of an ArticyRef.
     * @param Ref The ArticyRef to modify.
     * @param Object The UArticyObject to set as the reference.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Object", DefaultToSelf = "WorldContext"), Category = "ArticyRef")
    static void ArticyRef_SetObject(UPARAM(Ref) FArticyRef& Ref, UPARAM(Ref) UArticyObject* Object);

    /**
     * Converts an ArticyRef to an FArticyId.
     * @param Ref The ArticyRef to convert.
     * @return The FArticyId of the referenced object.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Object Id", DefaultToSelf = "WorldContext", BlueprintAutoCast), Category = "ArticyRef")
    static FArticyId ArticyRef_GetObjectId(FArticyRef Ref);

    /**
     * Sets the referenced object ID of an ArticyRef.
     * @param Ref The ArticyRef to modify.
     * @param Id The FArticyId to set.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Object Id", DefaultToSelf = "WorldContext", BlueprintAutoCast), Category = "ArticyRef")
    static void ArticyRef_SetObjectId(UPARAM(Ref) FArticyRef& Ref, UPARAM(Ref) FArticyId Id);

    /**
     * Compares two ArticyRefs for raw equality.
     * @param A The first ArticyRef.
     * @param B The second ArticyRef.
     * @return True if they are raw-equal.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ArtiycRef")
    static bool MatchesRaw(const FArticyRef& A, const FArticyRef& B);

    /**
     * Compares two ArticyRefs for effective equality.
     * @param A The first ArticyRef.
     * @param B The second ArticyRef.
     * @return True if they are effectively equal.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ArtiycRef")
    static bool MatchesEffective(const FArticyRef& A, const FArticyRef& B);

    /**
     * Converts an ArticyId to an object.
     * @param Id The ArticyId to convert.
     * @param CastTo The subclass type to cast the object to.
     * @param WorldContext The context within which the object is retrieved.
     * @return The UArticyObject referenced by the ArticyId.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Object", DefaultToSelf = "WorldContext", DeterminesOutputType = "CastTo"), Category = "ArticyId")
    static UArticyObject* ArticyId_GetObject(FArticyId Id, TSubclassOf<class UArticyObject> CastTo, const UObject* WorldContext);

    /**
     * Creates an ArticyId from a hex string.
     * @param hex The hex string to convert.
     * @return The ArticyId corresponding to the hex string.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "To Articy Id"), Category = "ArticyId")
    static FArticyId ArticyId_FromString(UPARAM(Ref) const FString& hex);

    /**
     * Gets the HEX string from an ID.
     * @param Id The ArticyId to convert.
     * @return The hex string representation of the ID.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "To Hex String"), Category = "ArticyId")
    static FString ArticyId_ToString(UPARAM(Ref) const FArticyId& Id);

    /**
     * Checks if two ArticyIds are equal.
     * @param A The first ArticyId.
     * @param B The second ArticyId.
     * @return True if the IDs are equal.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (ArticyId)", CompactNodeTitle = "==", Keywords = "== equal"), Category = "ArticyId")
    static bool ArticyId_Equal(const FArticyId& A, const FArticyId& B);

    /**
     * Checks if two ArticyIds are not equal.
     * @param A The first ArticyId.
     * @param B The second ArticyId.
     * @return True if the IDs are not equal.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (ArticyId)", CompactNodeTitle = "!=", Keywords = "!= not equal"), Category = "ArticyId")
    static bool ArticyId_NotEqual(const FArticyId& A, const FArticyId& B);

    /**
     * Checks if an ArticyId is valid.
     * @param Id The ArticyId to check.
     * @return True if the ID is valid.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Is Valid"), Category = "ArticyId")
    static bool ArticyId_IsValid(const FArticyId& Id);

    /**
     * Creates an ArticyGvName from a full name.
     * @param FullName The full name to use.
     * @return The corresponding ArticyGvName.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Make ArticyGvName from full name"), Category = "ArticyId")
    static FArticyGvName ArticyGvName_MakeFromFullName(const FName& FullName);

    /**
     * Creates an ArticyGvName from a variable and namespace.
     * @param Variable The variable name.
     * @param Namespace The namespace name.
     * @return The corresponding ArticyGvName.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Make ArticyGvName from namespace & variable"), Category = "ArticyId")
    static FArticyGvName ArticyGvName_MakeFromVariableAndNamespace(const FName& Variable, const FName& Namespace);

    /**
     * Gets the last object in a branch.
     * @param Branch The branch to analyze.
     * @return The target of the branch.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Target"), Category = "ArticyBranch")
    static TScriptInterface<class IArticyFlowObject> GetBranchTarget(UPARAM(ref) const struct FArticyBranch& Branch);

    /**
     * Gets the seen counter for a specific node.
     * @param Node The node to query.
     * @param WorldContext The context within which the counter is retrieved.
     * @return The seen counter value.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Seen Counter"), Category = "ArticyNode")
    static int GetNodeSeenCounter(TScriptInterface<class IArticyFlowObject> Node, const UObject* WorldContext);

    /**
     * Sets the seen counter for a specific node.
     * @param Node The node to modify.
     * @param Value The new seen counter value.
     * @param WorldContext The context within which the counter is set.
     * @return The updated seen counter value.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Set Seen Counter"), Category = "ArticyNode")
    static int SetNodeSeenCounter(TScriptInterface<class IArticyFlowObject> Node, int Value, const UObject* WorldContext);
};
