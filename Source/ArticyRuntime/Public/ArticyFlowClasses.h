//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPausableType.h"
#include "ArticyPins.h"
#include "Interfaces/ArticyNode.h"

#include "ArticyFlowClasses.generated.h"

/**
 * Represents a flow fragment in the Articy flow system.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyFlowFragment : public UArticyNode
{
    GENERATED_BODY()

public:

    /**
     * Returns the type of the node.
     * @return EArticyPausableType::FlowFragment
     */
    EArticyPausableType GetType() override { return EArticyPausableType::FlowFragment; }
};

/**
 * Represents a dialogue in the Articy flow system.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyDialogue : public UArticyNode
{
    GENERATED_BODY()

public:

    /**
     * Returns the type of the node.
     * @return EArticyPausableType::Dialogue
     */
    EArticyPausableType GetType() override { return EArticyPausableType::Dialogue; }
};

/**
 * Represents a dialogue fragment in the Articy flow system.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyDialogueFragment : public UArticyNode
{
    GENERATED_BODY()

public:

    /**
     * Returns the type of the node.
     * @return EArticyPausableType::DialogueFragment
     */
    EArticyPausableType GetType() override { return EArticyPausableType::DialogueFragment; }
};

/**
 * Represents a hub in the Articy flow system.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyHub : public UArticyNode
{
    GENERATED_BODY()

public:

    /**
     * Returns the type of the node.
     * @return EArticyPausableType::Hub
     */
    EArticyPausableType GetType() override { return EArticyPausableType::Hub; }
};

/**
 * Represents a jump in the Articy flow system.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyJump : public UArticyNode
{
    GENERATED_BODY()

public:

    /**
     * Returns the type of the node.
     * @return EArticyPausableType::Jump
     */
    EArticyPausableType GetType() override { return EArticyPausableType::Jump; }

    /**
     * Retrieves the target object of the jump.
     * @return Pointer to the target UArticyPrimitive object.
     */
    UArticyPrimitive* GetTarget() const;

    /**
     * Retrieves the ID of the target.
     * @return FArticyId representing the target ID.
     */
    FArticyId GetTargetID() const { return Target; }

    /**
     * Retrieves the target pin of the jump.
     * @return Pointer to the target UArticyFlowPin object.
     */
    UArticyFlowPin* GetTargetPin() const;

    /**
     * Retrieves the ID of the target pin.
     * @return FArticyId representing the target pin ID.
     */
    FArticyId GetTargetPinID() const { return TargetPin; }

    /**
     * Explores the flow from this jump to its target pin.
     * @param Player The flow player performing the exploration.
     * @param OutBranches The array to store resulting branches.
     * @param Depth The current depth of exploration.
     */
    void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override;

protected:

    /** The ID of the target pin. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Articy", meta = (DisplayName = "TargetPin"))
    FArticyId TargetPin;

    /** The ID of the target object. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Articy", meta = (DisplayName = "Target"))
    FArticyId Target;

private:

    /** Cached pointer to the target pin object. */
    UPROPERTY(VisibleAnywhere, Transient, Category = "Articy")
    mutable UArticyFlowPin* TargetPinObj;

    /** Cached pointer to the target object. */
    UPROPERTY(VisibleAnywhere, Transient, Category = "Articy")
    mutable UArticyPrimitive* TargetObj;
};
