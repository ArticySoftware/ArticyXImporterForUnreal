//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyFlowPlayer.h"
#include "ArticyRuntimeModule.h"
#include "Interfaces/ArticyFlowObject.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "ArticyExpressoScripts.h"
#include "UObject/ConstructorHelpers.h"
#include "Interfaces/ArticyInputPinsProvider.h"
#include "Interfaces/ArticyOutputPinsProvider.h"
#include "Engine/Texture2D.h"

/**
 * Retrieves the target of this branch.
 *
 * @return The target IArticyFlowObject of the branch, or nullptr if the branch is empty.
 */
TScriptInterface<IArticyFlowObject> FArticyBranch::GetTarget() const
{
    return Path.Num() > 0 ? Path.Last() : nullptr;
}

/**
 * Called when the game starts or when spawned.
 */
void UArticyFlowPlayer::BeginPlay()
{
    Super::BeginPlay();

    //update Cursor to object referenced by StartOn
    SetCursorToStartNode();

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UArticyFlowPlayer::OnTick), 0.0f);
}

/**
 * Called when the game ends or actor destroyed
 */
void UArticyFlowPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
    Super::EndPlay(EndPlayReason);
}

//---------------------------------------------------------------------------//

/**
 * Sets the start node for the flow player using an ArticyRef.
 *
 * @param StartNodeId The ID of the start node.
 */
void UArticyFlowPlayer::SetStartNode(FArticyRef StartNodeId)
{
    StartOn.SetId(StartNodeId.GetId());
    SetCursorToStartNode();
}

/**
 * Sets the start node for the flow player using a flow object.
 *
 * @param Node The flow object to start the flow player.
 */
void UArticyFlowPlayer::SetStartNodeWithFlowObject(TScriptInterface<IArticyFlowObject> Node)
{
    FArticyRef ArticyRef;
    ArticyRef.SetReference(Cast<UArticyObject>(Node.GetObject()));
    SetStartNode(ArticyRef);
}

/**
 * Sets the start node for the flow player using an ArticyId.
 *
 * @param NewId The new ID for the start node.
 */
void UArticyFlowPlayer::SetStartNodeById(FArticyId NewId)
{
    StartOn.SetId(NewId);
    SetCursorToStartNode();
}

/**
 * Sets the cursor to a specified node.
 *
 * @param Node The flow object to which the cursor should be set.
 */
void UArticyFlowPlayer::SetCursorTo(TScriptInterface<IArticyFlowObject> Node)
{
    if (!Node.GetObject())
    {
        UE_LOG(LogArticyRuntime, Warning, TEXT("Could not set cursor in flow player of actor %s: invalid node"), *this->GetOwner()->GetName());
        return;
    }

    Cursor = Node;
    UpdateAvailableBranchesInternal(true);
}

/**
 * Plays a specified branch by index.
 *
 * @param BranchIndex The index of the branch to play.
 */
void UArticyFlowPlayer::Play(int BranchIndex)
{
    static const TArray<FArticyBranch>* branchesPtr = &AvailableBranches;

    if (IgnoresInvalidBranches())
    {
        // Only create a new array if filtering is needed
        static TArray<FArticyBranch> ValidBranches;
        ValidBranches.Reset(); // Clear the array without reallocating memory

        // Filter valid branches
        for (const auto& branch : AvailableBranches)
        {
            if (branch.bIsValid)
            {
                ValidBranches.Add(branch);
            }
        }

        branchesPtr = &ValidBranches;
    }

    // Use branchesPtr instead of branches
    if (!branchesPtr->IsValidIndex(BranchIndex))
    {
        UE_LOG(LogArticyRuntime, Error, TEXT("Branch with index %d does not exist!"), BranchIndex);
        return;
    }

    PlayBranch((*branchesPtr)[BranchIndex]);
}

/**
 * Finishes the current paused object by executing a specified pin.
 *
 * @param PinIndex The index of the pin to execute.
 */
void UArticyFlowPlayer::FinishCurrentPausedObject(int PinIndex)
{
    IArticyOutputPinsProvider* outputPinOwner = Cast<IArticyOutputPinsProvider>(Cursor.GetObject());
    if (outputPinOwner)
    {
        auto outputPins = outputPinOwner->GetOutputPinsPtr();

        int numPins = outputPins->Num();
        if (numPins > 0 && PinIndex < numPins)
        {
            (*outputPins)[PinIndex]->Execute(GetGVs(), GetMethodsProvider());
        }
        if (outputPins->Num() > 0)
        {
            if (PinIndex < outputPins->Num())
            {
                (*outputPins)[PinIndex]->Execute(GetGVs(), GetMethodsProvider());
            }
            else
                UE_LOG(LogArticyRuntime, Warning, TEXT("FinishCurrentPausedObject: The index was out of bounds: Index: %d, PinCount: %d"), PinIndex, outputPins->Num());
        }
    }
}

/**
 * Determines whether the flow player should pause on a specific node.
 *
 * @param Node The node to check.
 * @return True if the flow player should pause on the node, otherwise false.
 */
bool UArticyFlowPlayer::ShouldPauseOn(IArticyFlowObject* Node) const
{
    return Node && (1 << static_cast<uint8>(Node->GetType()) & PauseOn);
}

/**
 * Determines whether the flow player should pause on a specific node.
 *
 * @param Node The node to check.
 * @return True if the flow player should pause on the node, otherwise false.
 */
bool UArticyFlowPlayer::ShouldPauseOn(TScriptInterface<IArticyFlowObject> Node) const
{
    return ShouldPauseOn(Cast<IArticyFlowObject>(Node.GetObject()));
}

/**
 * Retrieves the Articy database associated with this flow player.
 *
 * @return A pointer to the UArticyDatabase.
 */
UArticyDatabase* UArticyFlowPlayer::GetDB() const
{
    return UArticyDatabase::Get(this);
}

/**
 * Retrieves the global variables used by this flow player.
 *
 * @return A pointer to the UArticyGlobalVariables instance.
 */
UArticyGlobalVariables* UArticyFlowPlayer::GetGVs() const
{
    // If we have an custom GV set, make sure we're using a runtime clone of it
    if (OverrideGV) {
        return UArticyGlobalVariables::GetRuntimeClone(this, OverrideGV);
    }

    // Otherwise, use the default variable set
    return UArticyGlobalVariables::GetDefault(this);
}

/**
 * Retrieves the user methods provider for this flow player.
 *
 * @return A pointer to the methods provider object.
 */
UObject* UArticyFlowPlayer::GetMethodsProvider()
{
    if (!CachedExpressoInstance)
    {
        CachedExpressoInstance = GetDB()->GetExpressoInstance();
    }
    auto provider = CachedExpressoInstance->GetUserMethodsProviderInterface();

    if (ensure(provider))
    {
        //check if the set provider implements the required interface
        if (!UserMethodsProvider || !ensure(UserMethodsProvider->GetClass()->ImplementsInterface(provider)))
        {

            //no valid UserMethodsProvider set, search for it

            //check if the flow player itself implements it
            if (GetClass()->ImplementsInterface(provider))
                UserMethodsProvider = const_cast<UArticyFlowPlayer*>(this);
            else
            {
                auto actor = GetOwner();
                if (ensure(actor))
                {
                    //check if the flow player's owning actor implements it
                    if (actor->GetClass()->ImplementsInterface(provider))
                        UserMethodsProvider = actor;
                    else
                    {
                        //check if any other component implements it
                        auto components = actor->GetComponents();
                        for (auto comp : components)
                        {
                            if (comp->GetClass()->ImplementsInterface(provider))
                            {
                                UserMethodsProvider = comp;
                                break;
                            }
                        }

                        //and finally we check for a default methods provider, which we can use as fallback
                        auto defaultUserMethodsProvider = CachedExpressoInstance->GetDefaultUserMethodsProvider();
                        if (defaultUserMethodsProvider && ensure(defaultUserMethodsProvider->GetClass()->ImplementsInterface(provider)))
                            UserMethodsProvider = defaultUserMethodsProvider;
                    }
                }
            }
        }
    }

    return UserMethodsProvider;
}

/**
 * Retrieves the unshadowed node for a specified node.
 *
 * @param Node The node to unshadow.
 * @return A pointer to the unshadowed IArticyFlowObject.
 */
IArticyFlowObject* UArticyFlowPlayer::GetUnshadowedNode(IArticyFlowObject* Node)
{
    auto db = UArticyDatabase::Get(this);
    UArticyPrimitive* UnshadowedObject = db->GetObjectUnshadowed(Cast<UArticyPrimitive>(Node)->GetId());

    // Handle pins, because we can not request them directly from the db 
    if (!UnshadowedObject)
    {
        auto pinOwner = db->GetObjectUnshadowed(Cast<UArticyFlowPin>(Node)->GetOwner()->GetId());

        TArray<UArticyFlowPin*> pins;
        auto inputPinsOwner = Cast<IArticyInputPinsProvider>(pinOwner);
        if (inputPinsOwner)
        {
            pins.Append(*inputPinsOwner->GetInputPinsPtr());
        }
        auto outputPinsOwner = Cast<IArticyOutputPinsProvider>(pinOwner);
        if (outputPinsOwner)
        {
            pins.Append(*outputPinsOwner->GetOutputPinsPtr());
        }

        auto targetId = Cast<UArticyPrimitive>(Node)->GetId();
        for (auto pin : pins)
        {
            if (pin->GetId() == targetId)
            {
                UnshadowedObject = pin;
                break;
            }
        }
    }

    return Cast<IArticyFlowObject>(UnshadowedObject);
}

//---------------------------------------------------------------------------//

/**
 * Explores the flow starting from a specified node.
 *
 * @param Node The node to start exploring from.
 * @param bShadowed Whether the exploration should be shadowed.
 * @param Depth The current depth of exploration.
 * @param IncludeCurrent Whether to include the current node in the exploration.
 * @return An array of branches resulting from the exploration.
 */
TArray<FArticyBranch> UArticyFlowPlayer::Explore(IArticyFlowObject* Node, bool bShadowed, int32 Depth, bool IncludeCurrent)
{
    TArray<FArticyBranch> OutBranches;

    //check stop condition
    if ((Depth > ExploreLimit || !Node || (Node != Cursor.GetInterface() && ShouldPauseOn(Node))))
    {
        if (Depth > ExploreLimit)
            UE_LOG(LogArticyRuntime, Warning, TEXT("ExploreDepthLimit (%d) reached, stopping exploration!"), ExploreLimit);
        if (!Node)
            UE_LOG(LogArticyRuntime, Warning, TEXT("Found a nullptr Node when exploring a branch!"));

        /* TODO where should I put this?
        if(OutBranches.Num() >= BranchLimit)
        {
            UE_LOG(LogArticyRuntime, Warning, TEXT("BranchLimit (%d) reached, cannot add another branch!"), BranchLimit);
            return;
        }*/

        //target reached, create a branch
        auto branch = FArticyBranch{};
        if (Node)
        {
            /* NOTE: This check must not be done, as the last node in a branch never affects
            * validity of the branch. A branch is only invalidated if it runs THROUGH a node
            * with invalid condition, instead of just UP TO that node.
            branch.bIsValid = Node->Execute(this); */

            auto unshadowedNode = GetUnshadowedNode(Node);

            TScriptInterface<IArticyFlowObject> ptr;
            ptr.SetObject(unshadowedNode->_getUObject());
            ptr.SetInterface(unshadowedNode);
            branch.Path.Add(ptr);
        }

        OutBranches.Add(branch);
    }
    else
    {
        //set speaker on expresso scripts
        auto xp = GetDB()->GetExpressoInstance();
        if (ensure(xp))
        {
            auto obj = Cast<UArticyPrimitive>(Node);
            if (obj)
            {
                xp->SetCurrentObject(obj);

                IArticyObjectWithSpeaker* speaker;
                if (auto flowPin = Cast<UArticyFlowPin>(Node))
                    speaker = Cast<IArticyObjectWithSpeaker>(flowPin->GetOwner());
                else
                    speaker = Cast<IArticyObjectWithSpeaker>(obj);

                if (speaker)
                    xp->SetSpeaker(speaker->GetSpeaker());
            }
        }

        //if this is the first node, try to submerge
        bool bSubmerged = false;
        if (Depth == 0)
        {
            auto inputPinProvider = Cast<IArticyInputPinsProvider>(Node);
            if (inputPinProvider)
                bSubmerged = inputPinProvider->TrySubmerge(this, OutBranches, Depth + 1, bShadowed); //NOTE: bShadowed will always be true if Depth == 0
        }

        //explore this node
        if (!bSubmerged)
        {
            if (bShadowed)
            {
                //explore the node inside a shadowed operation
                ShadowedOperation([&] { Node->Explore(this, OutBranches, Depth + 1); });
            }
            else
            {
                //non-shadowed explore
                Node->Explore(this, OutBranches, Depth + 1);
            }
        }

        // add this node to the head of all the branches
        // 
        // Only do this if IncludeCurrent is true. 
        // See https://github.com/ArticySoftware/ArticyImporterForUnreal/issues/50
        if (IncludeCurrent)
        {
            for (auto& branch : OutBranches)
            {
                auto unshadowedNode = GetUnshadowedNode(Node);
                TScriptInterface<IArticyFlowObject> ptr;
                ptr.SetObject(unshadowedNode->_getUObject());
                ptr.SetInterface(unshadowedNode);

                branch.Path.Insert(ptr, 0); //TODO inserting at front is not ideal performance wise
            }
        }
    }

    return OutBranches;
}

/**
 * Sets the nodes on which the flow player should pause.
 *
 * @param Types The types of nodes to pause on.
 */
void UArticyFlowPlayer::SetPauseOn(EArticyPausableType Types)
{
    PauseOn = 1 << uint8(Types & EArticyPausableType::DialogueFragment)
        | 1 << uint8(Types & EArticyPausableType::Dialogue)
        | 1 << uint8(Types & EArticyPausableType::FlowFragment)
        | 1 << uint8(Types & EArticyPausableType::Hub)
        | 1 << uint8(Types & EArticyPausableType::Jump)
        | 1 << uint8(Types & EArticyPausableType::Condition)
        | 1 << uint8(Types & EArticyPausableType::Instruction)
        | 1 << uint8(Types & EArticyPausableType::Pin);
}

//---------------------------------------------------------------------------//

/**
 * Updates the available branches from the current cursor position.
 */
void UArticyFlowPlayer::UpdateAvailableBranches()
{
    UpdateAvailableBranchesInternal(false);
}

bool UArticyFlowPlayer::OnTick(float DeltaTime)
{
    FArticyBranch Branch;
    while (BranchQueue.Dequeue(Branch))
    {
        if (!ensure(ShadowLevel == 0))
        {
            UE_LOG(LogArticyRuntime, Error, TEXT("ArticyFlowPlayer::Traverse was called inside a ShadowedOperation! Aborting Play."))
                return true;
        }

        for (auto& node : Branch.Path)
        {
            node->Execute(GetGVs(), GetMethodsProvider());

            // update nodes visited
            auto* GVs = GetGVs();
            if (GVs)
            {
                GVs->IncrementSeenCounter(Cast<IArticyFlowObject>(node.GetObject()));
            }
        }

        Cursor = Branch.Path.Last();
        UpdateAvailableBranches();
    }
    return true;
}

//---------------------------------------------------------------------------//

/**
 * Updates the available branches from the current cursor position.
 *
 * @param Startup Whether this is a startup update.
 */
void UArticyFlowPlayer::UpdateAvailableBranchesInternal(bool Startup)
{
    AvailableBranches.Reset();

    if (PauseOn == 0)
        UE_LOG(LogArticyRuntime, Warning, TEXT("PauseOn is not set, not exploring the Flow as it would not pause on any node."))
    else if (!Cursor)
        UE_LOG(LogArticyRuntime, Warning, TEXT("Cannot explore flow, cursor is not set!"))
    else
    {
        const bool bMustBeShadowed = true;
        AvailableBranches = Explore(&*Cursor, bMustBeShadowed, 0, Startup);

        // Prune empty branches
        AvailableBranches.RemoveAllSwap([](const FArticyBranch& branch) { return branch.Path.Num() == 0; });

        if (AvailableBranches.IsEmpty())
        {
            // no valid branches, check for fallback
            auto* GVs = GetGVs();
            GVs->SetFallbackEvaluation(&*Cursor, true);
            auto WithFallback = Explore(&*Cursor, bMustBeShadowed, 0, Startup);
            GVs->SetFallbackEvaluation(&*Cursor, false);
            AvailableBranches = WithFallback;
        }

        // NP: Every branch needs the index so that Play() can actually take a branch as input
        for (int32 i = 0; i < AvailableBranches.Num(); i++)
            AvailableBranches[i].Index = i;

        // If we're just starting up, check if we should fast-forward
        if (Startup && FastForwardToPause())
        {
            //fast-forwarding will call UpdateAvailableBranches again, can abort here
            return;
        }

        //broadcast and return result
        OnPlayerPaused.Broadcast(Cursor);
        OnBranchesUpdated.Broadcast(AvailableBranches);
    }
}

/**
 * Sets the cursor to the start node.
 */
void UArticyFlowPlayer::SetCursorToStartNode()
{
    // This ensure Flowplayer construction whithout Throwing
    // error message when setup in Actor construction with C++
    if (StartOn.NoneSet)
    {
        return;
    }

    const auto& obj = StartOn.GetObject(this);

    TScriptInterface<IArticyFlowObject> ptr;
    ptr.SetObject(obj);
    ptr.SetInterface(Cast<IArticyFlowObject>(obj));

    SetCursorTo(ptr);
}

/**
 * Fast-forwards the flow to the next pause point.
 *
 * @return True if the flow was fast-forwarded, otherwise false.
 */
bool UArticyFlowPlayer::FastForwardToPause()
{
    checkNoRecursion(); //this cannot recurse!

    if (AvailableBranches.Num() <= 0)
        return false;

    const auto& firstPath = AvailableBranches[0].Path;
    if (!ensure(firstPath.Num() > 0))
        return false;

    int ffwdIndex;
    for (ffwdIndex = 0; ffwdIndex < firstPath.Num(); ++ffwdIndex)
    {
        const auto& node = firstPath[ffwdIndex];
        if (ShouldPauseOn(&*node))
        {
            //pause on this node
            break;
        }

        auto bSplitFound = false;
        for (int b = 1; b < AvailableBranches.Num(); ++b)
        {
            const auto& path = AvailableBranches[b].Path;
            //it shouldn't be possible that one path is a subset of the other one
            //(shorter but all nodes equal to the other path)
            if (!ensure(path.IsValidIndex(ffwdIndex)) || path[ffwdIndex] != node)
            {
                bSplitFound = true;
                break;
            }
        }

        if (bSplitFound)
        {
            //pause on the node BEFORE
            --ffwdIndex;
            break;
        }
    }

    if (ffwdIndex < 0 || ffwdIndex >= firstPath.Num())
    {
        //no need to fast-forward
        return false;
    }

    //create the fast-forward branch
    auto newBranch = FArticyBranch{};
    newBranch.bIsValid = AvailableBranches[0].bIsValid;
    for (int i = 0; i <= ffwdIndex; ++i)
    {
        //add node to branch
        newBranch.Path.Add(firstPath[i]);
    }

    //this also calls UpdateAvailableBranches again
    PlayBranch(newBranch);

    return true;
}

/**
 * Plays a specified branch, executing all nodes within it.
 *
 * @param Branch The branch to play.
 */
void UArticyFlowPlayer::PlayBranch(const FArticyBranch& Branch)
{
    BranchQueue.Enqueue(Branch);
}

/**
 * Constructor for AArticyFlowDebugger class.
 */
AArticyFlowDebugger::AArticyFlowDebugger()
{
    FlowPlayer = CreateDefaultSubobject<UArticyFlowPlayer>(TEXT("Articy Flow Player"));
    ArticyImporterIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));

    auto ImporterIconFinder = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("Texture2D'/ArticyXImporter/Res/ArticyImporter64.ArticyImporter64'"));
    ArticyImporterIcon->SetSprite(ImporterIconFinder.Object);
    FlowPlayer->SetIgnoreInvalidBranches(false);
}
