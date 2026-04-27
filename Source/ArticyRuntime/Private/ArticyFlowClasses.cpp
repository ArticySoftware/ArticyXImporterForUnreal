//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyFlowClasses.h"

#include "ArticyBranch.h"
#include "ArticyDatabase.h"
#include "ArticyFlowPlayer.h"

/**
 * Retrieves the target object of the jump.
 *
 * This function will return the target object if it has been resolved,
 * otherwise, it will attempt to resolve it from the Articy database.
 *
 * @return Pointer to the target UArticyPrimitive object.
 */
UArticyPrimitive* UArticyJump::GetTarget() const
{
    if (!TargetObj)
    {
        auto db = UArticyDatabase::Get(this);
        TargetObj = db ? db->GetObject(Target) : nullptr;
    }

    return TargetObj;
}

/**
 * Retrieves the target pin of the jump.
 *
 * This function will return the target pin object if it has been resolved,
 * otherwise, it will attempt to resolve it from the target object.
 *
 * @return Pointer to the target UArticyFlowPin object.
 */
UArticyFlowPin* UArticyJump::GetTargetPin() const
{
    if (!TargetPinObj)
    {
        auto target = GetTarget();
        if (target)
            TargetPinObj = Cast<UArticyFlowPin>(target->GetSubobject(TargetPin));
    }

    return TargetPinObj;
}

/**
 * Explores the flow from this jump to its target pin.
 *
 * This function adds branches to the OutBranches array based on the exploration
 * starting from the target pin.
 *
 * @param Player The flow player performing the exploration.
 * @param OutBranches The array to store resulting branches.
 * @param Depth The current depth of exploration.
 */
void UArticyJump::Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth)
{
    // NOTE: Even though a Jump also has an OutputPins member because of inheritance,
    // it never really has output pins! Instead, the jump target is specified by the
    // TargetPin (and Target) member.

    auto pin = GetTargetPin();
    if (pin)
    {
        const auto bShadowed = false;
        OutBranches.Append(Player->Explore(pin, bShadowed, Depth + 1));
    }
    else
    {
        // DEAD-END!
        OutBranches.Add(FArticyBranch{});
    }
}
