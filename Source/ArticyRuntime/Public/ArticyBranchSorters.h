//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyBranch.h"
#include "ArticyBuiltinTypes.h"
#include "Interfaces/ArticyInputPinsProvider.h"
#include "Interfaces/ArticyObjectWithPosition.h"

namespace ArticyBranchSorters
{
    /**
     * Sorts branches according to their target's:
     * 
     * - position in the flow graph, if the target is a positional object or an input pin with a target that is a positional object
     * - index of an input pin
     * - position of an outgoing connection's target
     */
    struct FArticyBranchPositionSorter
    {
        
        FORCEINLINE bool operator()(const FArticyBranch& A, const FArticyBranch& B) const
        {
            const IArticyObjectWithPosition* APos = Cast<IArticyObjectWithPosition>(A.GetTarget().GetInterface());
            const IArticyObjectWithPosition* BPos = Cast<IArticyObjectWithPosition>(B.GetTarget().GetInterface());
        
            if (APos != nullptr && BPos != nullptr)
            {
                return IsPositionSmaller(APos, BPos);
            }
            
            const UArticyInputPin* AInputPin = Cast<UArticyInputPin>(A.GetTarget().GetInterface());
            const UArticyInputPin* BInputPin = Cast<UArticyInputPin>(B.GetTarget().GetInterface());
            
            if (AInputPin != nullptr && BInputPin != nullptr)
            {
                return IsInputPinSmaller(AInputPin, BInputPin);
            }
            
            const UArticyOutgoingConnection* AOutgoingConnection = Cast<UArticyOutgoingConnection>(A.GetTarget().GetInterface());
            const UArticyOutgoingConnection* BOutgoingConnection = Cast<UArticyOutgoingConnection>(B.GetTarget().GetInterface());
            
            if (AOutgoingConnection != nullptr && BOutgoingConnection != nullptr)
            {
                return IsOutgoingConnectionSmaller(AOutgoingConnection, BOutgoingConnection);
            }
        
            return true;
        }
        
        FORCEINLINE static bool IsPositionSmaller(const IArticyObjectWithPosition* APosObj, const IArticyObjectWithPosition* BPosObj)
        {
            if (APosObj != nullptr && BPosObj != nullptr)
            {
                const FVector2D APos2D = APosObj->GetPosition();
                const FVector2D BPos2D = BPosObj->GetPosition();
            
                return APos2D.X == BPos2D.X ? APos2D.Y < BPos2D.Y : APos2D.X < BPos2D.X;
            }
            
            return true;
        }
        
        FORCEINLINE static bool IsInputPinSmaller(const UArticyInputPin* AInputPin, const UArticyInputPin* BInputPin)
        {
            if (AInputPin != nullptr && BInputPin != nullptr)
            {
                UArticyObject* AOwner = UArticyObject::FindAsset(AInputPin->Owner);
                UArticyObject* BOwner = UArticyObject::FindAsset(BInputPin->Owner);
                
                const IArticyObjectWithPosition* AOwnerPos = Cast<IArticyObjectWithPosition>(AOwner);
                const IArticyObjectWithPosition* BOwnerPos = Cast<IArticyObjectWithPosition>(BOwner);
                
                if (AOwnerPos != nullptr && BOwnerPos != nullptr && AOwnerPos != BOwnerPos)
                {
                    return IsPositionSmaller(AOwnerPos, BOwnerPos);
                }
                
                const IArticyInputPinsProvider* AInputPinsProvider = Cast<IArticyInputPinsProvider>(AOwner);
                const IArticyInputPinsProvider* BInputPinsProvider = Cast<IArticyInputPinsProvider>(BOwner);
                
                if (AInputPinsProvider != nullptr && BInputPinsProvider != nullptr)
                {
                    return AInputPinsProvider->GetInputPins().IndexOfByKey(AInputPin) < BInputPinsProvider->GetInputPins().IndexOfByKey(BInputPin);
                }
            }
            
            return true;
        }
        
        FORCEINLINE static bool IsOutgoingConnectionSmaller(const UArticyOutgoingConnection* AOutgoingConnection, const UArticyOutgoingConnection* BOutgoingConnection)
        {
            const IArticyObjectWithPosition* APosObj = Cast<IArticyObjectWithPosition>(AOutgoingConnection->GetTarget());
            const IArticyObjectWithPosition* BPosObj = Cast<IArticyObjectWithPosition>(BOutgoingConnection->GetTarget());
            
            if (APosObj != nullptr && BPosObj != nullptr && APosObj != BPosObj)
            {
                return IsPositionSmaller(APosObj, BPosObj);
            }
            
            return true;
        }
    };
}
