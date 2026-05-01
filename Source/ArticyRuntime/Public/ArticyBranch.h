//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Interfaces/ArticyFlowObject.h"

#include "ArticyBranch.generated.h"

/**
 * Represents a branch in the Articy flow, which consists of a path of nodes.
 */
USTRUCT(BlueprintType)
struct ARTICYRUNTIME_API FArticyBranch
{
    GENERATED_BODY()

public:

    /**
     * The list of nodes this branch contains.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
    TArray<TScriptInterface<IArticyFlowObject>> Path;

    /** This is true if all conditions along the path evaluate to true. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
    bool bIsValid = true;

    /** The index of this branch in the owning flow player's AvailableBranches array. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
    int32 Index = -1;

    /** Retrieve the last object in the path. */
    TScriptInterface<IArticyFlowObject> GetTarget() const;
};
