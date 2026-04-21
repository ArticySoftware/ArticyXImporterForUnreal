//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyBranch.h"
#include "ArticyFunctionLibrary.h"
#include "ArticyObject.h"
#include "ArticyFlowPlayer.h"
#include "Interfaces/ArticyFlowObject.h"

/**
 * Retrieves the object referenced by an ArticyRef.
 * @param Ref The ArticyRef to get the object from.
 * @param CastTo The subclass type to cast the object to.
 * @param WorldContext The context within which the object is retrieved.
 * @return The referenced UArticyObject.
 */
UArticyObject* UArticyFunctionLibrary::ArticyRef_GetObject(FArticyRef Ref, TSubclassOf<class UArticyObject> CastTo, const UObject* WorldContext)
{
    return Ref.GetObject(WorldContext);
}

/**
 * Sets the object reference of an ArticyRef.
 * @param Ref The ArticyRef to set the object for.
 * @param Object The UArticyObject to set as reference.
 */
void UArticyFunctionLibrary::ArticyRef_SetObject(FArticyRef& Ref, UArticyObject* Object)
{
    Ref.SetReference(Object);
}

/**
 * Gets the ID of the object referenced by an ArticyRef.
 * @param Ref The ArticyRef to get the object ID from.
 * @return The ID of the referenced object.
 */
FArticyId UArticyFunctionLibrary::ArticyRef_GetObjectId(FArticyRef Ref)
{
    return Ref.GetId();
}

/**
 * Sets the ID of the object referenced by an ArticyRef.
 * @param Ref The ArticyRef to set the object ID for.
 * @param Id The ID to set for the reference.
 */
void UArticyFunctionLibrary::ArticyRef_SetObjectId(FArticyRef& Ref, FArticyId Id)
{
    Ref.SetId(Id);
}

/**
 * Compares two ArticyRefs for raw equality.
 * @param A The first ArticyRef.
 * @param B The second ArticyRef.
 * @return True if they are equal in raw terms.
 */
bool UArticyFunctionLibrary::MatchesRaw(const FArticyRef& A, const FArticyRef& B)
{
    return A.MatchesRaw(B);
}

/**
 * Compares two ArticyRefs for effective equality.
 * @param A The first ArticyRef.
 * @param B The second ArticyRef.
 * @return True if they are effectively equal.
 */
bool UArticyFunctionLibrary::MatchesEffective(const FArticyRef& A, const FArticyRef& B)
{
    return A.MatchesEffective(B);
}

/**
 * Retrieves the object referenced by an ArticyId.
 * @param Id The ArticyId to get the object from.
 * @param CastTo The subclass type to cast the object to.
 * @param WorldContext The context within which the object is retrieved.
 * @return The referenced UArticyObject.
 */
UArticyObject* UArticyFunctionLibrary::ArticyId_GetObject(FArticyId Id, TSubclassOf<class UArticyObject> CastTo, const UObject* WorldContext)
{
    return Cast<UArticyObject>(Id.GetObject(WorldContext));
}

/**
 * Creates an ArticyId from a hex string.
 * @param hex The hex string to convert.
 * @return The corresponding ArticyId.
 */
FArticyId UArticyFunctionLibrary::ArticyId_FromString(const FString& hex)
{
    return FArticyId{ hex };
}

/**
 * Converts an ArticyId to a hex string.
 * @param Id The ArticyId to convert.
 * @return The hex string representation of the ID.
 */
FString UArticyFunctionLibrary::ArticyId_ToString(const FArticyId& Id)
{
    return ArticyHelpers::Uint64ToHex(Id);
}

/**
 * Checks if two ArticyIds are equal.
 * @param A The first ArticyId.
 * @param B The second ArticyId.
 * @return True if both IDs are equal.
 */
bool UArticyFunctionLibrary::ArticyId_Equal(const FArticyId& A, const FArticyId& B)
{
    return A == B;
}

/**
 * Checks if two ArticyIds are not equal.
 * @param A The first ArticyId.
 * @param B The second ArticyId.
 * @return True if both IDs are not equal.
 */
bool UArticyFunctionLibrary::ArticyId_NotEqual(const FArticyId& A, const FArticyId& B)
{
    return A != B;
}

/**
 * Checks if an ArticyId is valid.
 * @param Id The ArticyId to check.
 * @return True if the ID is valid.
 */
bool UArticyFunctionLibrary::ArticyId_IsValid(const FArticyId& Id)
{
    return Id && Id.Low != 0 && Id.High != 0;
}

/**
 * Creates an ArticyGvName from a full name.
 * @param FullName The full name to use.
 * @return The corresponding ArticyGvName.
 */
FArticyGvName UArticyFunctionLibrary::ArticyGvName_MakeFromFullName(const FName& FullName)
{
    return FArticyGvName(FullName);
}

/**
 * Creates an ArticyGvName from a variable and namespace.
 * @param Variable The variable name.
 * @param Namespace The namespace name.
 * @return The corresponding ArticyGvName.
 */
FArticyGvName UArticyFunctionLibrary::ArticyGvName_MakeFromVariableAndNamespace(const FName& Variable, const FName& Namespace)
{
    return FArticyGvName(Variable, Namespace);
}

/**
 * Retrieves the target of a given ArticyBranch.
 * @param Branch The branch to get the target from.
 * @return The target flow object.
 */
TScriptInterface<IArticyFlowObject> UArticyFunctionLibrary::GetBranchTarget(const FArticyBranch& Branch)
{
    return Branch.GetTarget();
}

/**
 * Gets the seen counter for a specific node.
 * @param Node The node to get the seen counter for.
 * @param WorldContext The context within which the counter is retrieved.
 * @return The seen counter value.
 */
int UArticyFunctionLibrary::GetNodeSeenCounter(TScriptInterface<IArticyFlowObject> Node, const UObject* WorldContext)
{
    auto* db = UArticyDatabase::Get(WorldContext);
    if (!db)
    {
        return 0;
    }
    auto* GVs = db->GetGVs();
    if (GVs)
    {
        return GVs->GetSeenCounter(Cast<IArticyFlowObject>(Node.GetObject()));
    }

    return 0;
}

/**
 * Sets the seen counter for a specific node.
 * @param Node The node to set the seen counter for.
 * @param Value The value to set for the seen counter.
 * @param WorldContext The context within which the counter is set.
 * @return The updated seen counter value.
 */
int UArticyFunctionLibrary::SetNodeSeenCounter(TScriptInterface<IArticyFlowObject> Node, int Value, const UObject* WorldContext)
{
    auto* db = UArticyDatabase::Get(WorldContext);
    if (!db)
    {
        return 0;
    }
    auto* GVs = db->GetGVs();
    if (GVs)
    {
        return GVs->SetSeenCounter(Cast<IArticyFlowObject>(Node.GetObject()), Value);
    }

    return 0;
}
