//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyFlowPlayer.h"
#include "ArticyRuntimeModule.h"
#include "Interfaces/ArticyFlowObject.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "ArticyExpressoScripts.h"
#include "ArticyAlternativeGlobalVariables.h"
#include "UObject/ConstructorHelpers.h"
#include "Interfaces/ArticyInputPinsProvider.h"
#include "Interfaces/ArticyOutputPinsProvider.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "ArticyFlowPlayer"

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

    //report a broken setup once, so it does not have to be hunted down from the symptoms
    for (const auto& issue : ValidateSetup())
    {
        if (issue.Severity == EArticyFlowPlayerIssueSeverity::Error)
            UE_LOG(LogArticyRuntime, Error, TEXT("ArticyFlowPlayer on %s: %s"), *GetNameSafe(GetOwner()), *issue.Message.ToString())
        else
            UE_LOG(LogArticyRuntime, Warning, TEXT("ArticyFlowPlayer on %s: %s"), *GetNameSafe(GetOwner()), *issue.Message.ToString())
    }

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
        UE_LOG(LogArticyRuntime, Warning, TEXT("Could not set cursor in flow player of actor %s: invalid node"), *GetNameSafe(GetOwner()));
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
    // IsValid rather than a null check: also re-fetches an instance that was marked as garbage
    if (!IsValid(CachedExpressoInstance))
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
    auto primitive = Cast<UArticyPrimitive>(Node);
    if (!primitive)
        return nullptr;

    auto db = UArticyDatabase::Get(this);
    UArticyPrimitive* UnshadowedObject = db ? db->GetObjectUnshadowed(primitive->GetId()) : nullptr;

    // Handle pins and connections, because we can not request them directly from the db
    if (!UnshadowedObject)
    {
        if (auto connection = Cast<UArticyOutgoingConnection>(primitive))
            UnshadowedObject = GetUnshadowedConnection(connection);
        else if (auto pin = Cast<UArticyFlowPin>(primitive))
            UnshadowedObject = GetUnshadowedPin(pin);
    }

    return Cast<IArticyFlowObject>(UnshadowedObject);
}

/**
 * Retrieves the unshadowed counterpart of a pin.
 *
 * @param Pin The pin to unshadow.
 * @return The pin of the unshadowed owner that carries the same id, or nullptr.
 */
UArticyFlowPin* UArticyFlowPlayer::GetUnshadowedPin(UArticyFlowPin* Pin)
{
    auto db = UArticyDatabase::Get(this);
    auto owner = Pin ? Pin->GetOwner() : nullptr;
    if (!db || !owner)
        return nullptr;

    auto pinOwner = db->GetObjectUnshadowed(owner->GetId());

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

    const auto targetId = Pin->GetId();
    for (auto pin : pins)
    {
        if (pin->GetId() == targetId)
            return pin;
    }

    return nullptr;
}

/**
 * Retrieves the unshadowed counterpart of a connection.
 *
 * Connections are not exported with an id, so they are matched by their position on the
 * unshadowed owning pin, whose connections are in the same order.
 *
 * @param Connection The connection to unshadow.
 * @return The connection at the same index of the unshadowed owning pin, or nullptr.
 */
UArticyOutgoingConnection* UArticyFlowPlayer::GetUnshadowedConnection(UArticyOutgoingConnection* Connection)
{
    auto ownerPin = Connection ? Cast<UArticyFlowPin>(Connection->GetOuter()) : nullptr;
    if (!ownerPin)
        return nullptr;

    const int32 index = ownerPin->Connections.IndexOfByKey(Connection);
    if (index == INDEX_NONE)
        return nullptr;

    auto unshadowedPin = GetUnshadowedPin(ownerPin);
    if (!unshadowedPin || !unshadowedPin->Connections.IsValidIndex(index))
        return nullptr;

    return unshadowedPin->Connections[index];
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
            ptr.SetObject(Cast<UObject>(unshadowedNode));
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
                ptr.SetObject(Cast<UObject>(unshadowedNode));
                ptr.SetInterface(unshadowedNode);

                branch.Path.Insert(ptr, 0); //TODO inserting at front is not ideal performance wise
            }
        }
    }

    return OutBranches;
}

/**
 * Sets the single node type the flow player should pause on.
 *
 * @param Type The type of node to pause on.
 */
void UArticyFlowPlayer::SetPauseOn(EArticyPausableType Type)
{
    PauseOn = ArticyPauseOnFlag(Type);
}

/**
 * Sets the nodes on which the flow player should pause.
 *
 * @param Types Bitmask of EArticyPausableType values, as PauseOn stores it.
 */
void UArticyFlowPlayer::SetPauseOnMask(int32 Types)
{
    PauseOn = Types;
}

//---------------------------------------------------------------------------//

/**
 * Updates the available branches from the current cursor position.
 */
void UArticyFlowPlayer::UpdateAvailableBranches()
{
    UpdateAvailableBranchesInternal(false);
}

/**
 * Re-explores the available branches without pausing on the cursor again.
 */
void UArticyFlowPlayer::RefreshBranches()
{
    UpdateAvailableBranchesInternal(false, false);
}

/**
 * Broadcasts OnPlayerPaused for the current cursor.
 */
void UArticyFlowPlayer::BroadcastPlayerPaused()
{
    OnPlayerPaused.Broadcast(Cursor);
}

/**
 * Broadcasts OnBranchesUpdated for the current branches.
 */
void UArticyFlowPlayer::BroadcastBranchesUpdated()
{
    OnBranchesUpdated.Broadcast(AvailableBranches);
}

/**
 * Advances the continuous recalculation timer.
 *
 * @param DeltaTime The time elapsed since the last tick.
 * @return True if the interval elapsed, in which case the timer starts over.
 */
bool UArticyFlowPlayer::AdvanceRecalculationTimer(float DeltaTime)
{
    if (!bContinuouslyRecalculateBranches)
    {
        TimeSinceLastRecalculation = 0.f;
        return false;
    }

    TimeSinceLastRecalculation += DeltaTime;
    if (TimeSinceLastRecalculation < ContinuouslyRecalculateInterval)
        return false;

    TimeSinceLastRecalculation = 0.f;
    return true;
}

/**
 * Collects everything about this flow player's setup that looks wrong.
 *
 * @return One issue per problem found, empty if the setup looks fine.
 */
TArray<FArticyFlowPlayerSetupIssue> UArticyFlowPlayer::ValidateSetup() const
{
    TArray<FArticyFlowPlayerSetupIssue> Issues;

    const auto AddIssue = [&Issues](EArticyFlowPlayerIssueSeverity Severity, FName PropertyName, FText Message)
    {
        FArticyFlowPlayerSetupIssue Issue;
        Issue.Severity = Severity;
        Issue.PropertyName = PropertyName;
        Issue.Message = MoveTemp(Message);
        Issues.Add(MoveTemp(Issue));
    };

    if (StartOn.GetId().IsNull())
    {
        // Only a warning: plenty of projects leave this empty and call Set Start Node at runtime
        AddIssue(EArticyFlowPlayerIssueSeverity::Warning, GET_MEMBER_NAME_CHECKED(UArticyFlowPlayer, StartOn),
            LOCTEXT("NoStartOn", "Start On is not set, so the flow player starts nowhere. Pick the node the flow should start on, or call Set Start Node at runtime."));
    }

    if (PauseOn == 0)
    {
        AddIssue(EArticyFlowPlayerIssueSeverity::Error, GET_MEMBER_NAME_CHECKED(UArticyFlowPlayer, PauseOn),
            LOCTEXT("NoPauseOn", "Pause On is empty. The flow player would not pause on any node, so the flow is not explored at all."));
    }

    if (OverrideGV && !IsValid(OverrideGV))
    {
        AddIssue(EArticyFlowPlayerIssueSeverity::Error, GET_MEMBER_NAME_CHECKED(UArticyFlowPlayer, OverrideGV),
            LOCTEXT("InvalidOverrideGV", "Override GV references a missing asset. Clear it to use the shared global variables, or point it at an Articy Alternative Global Variables asset."));
    }

    // The interface the project's script methods are declared on; without it there is nothing to check against
    if (UClass* ProviderInterface = GetScriptMethodProviderInterface())
    {
        const FText InterfaceName = FText::FromString(ProviderInterface->GetName());

        if (UserMethodsProvider && !UserMethodsProvider->GetClass()->ImplementsInterface(ProviderInterface))
        {
            AddIssue(EArticyFlowPlayerIssueSeverity::Error, GET_MEMBER_NAME_CHECKED(UArticyFlowPlayer, UserMethodsProvider),
                FText::Format(LOCTEXT("InvalidMethodsProvider", "User Methods Provider does not implement {0}, so script methods cannot be resolved from it. Implement the interface on it or clear the property to let the flow player search for a provider."), InterfaceName));
        }
        else if (!UserMethodsProvider && !HasScriptMethodProviderInReach(ProviderInterface))
        {
            AddIssue(EArticyFlowPlayerIssueSeverity::Warning, GET_MEMBER_NAME_CHECKED(UArticyFlowPlayer, UserMethodsProvider),
                FText::Format(LOCTEXT("NoMethodsProvider", "Nothing on this actor implements {0}. Script methods authored in articy fall back to the default provider, which logs a warning for every call."), InterfaceName));
        }
    }

    // Only reported once the flow player actually paused with nothing bound: at begin play
    // the owner has not had its chance to bind yet
    if (bPausedWithoutCallbacks && !OnPlayerPaused.IsBound() && !OnBranchesUpdated.IsBound())
    {
        AddIssue(EArticyFlowPlayerIssueSeverity::Warning, NAME_None,
            LOCTEXT("NoCallbacks", "Nothing is bound to On Player Paused or On Branches Updated, so the traversed flow is not reported anywhere."));
    }

    return Issues;
}

/**
 * Retrieves the interface the project's script methods are declared on.
 *
 * Resolved from the database asset rather than from a clone, so this is safe to call in the
 * editor, outside of play.
 *
 * @return The generated methods provider interface, or nullptr if the project has no script methods.
 */
UClass* UArticyFlowPlayer::GetScriptMethodProviderInterface() const
{
    auto original = UArticyDatabase::GetMutableOriginal();
    if (!original.IsValid())
        return nullptr;

    auto expressoClass = original->GetExpressoScriptsClass();
    if (!expressoClass)
        return nullptr;

    auto expresso = expressoClass->GetDefaultObject<UArticyExpressoScripts>();
    return expresso ? expresso->GetUserMethodsProviderInterface() : nullptr;
}

/**
 * Checks whether the flow player would find a script methods provider on its own.
 *
 * Mirrors the search order of GetMethodsProvider, without the default provider fallback.
 *
 * @param ProviderInterface The interface a provider has to implement.
 * @return True if the flow player, its owning actor or one of the actor's components implements it.
 */
bool UArticyFlowPlayer::HasScriptMethodProviderInReach(UClass* ProviderInterface) const
{
    if (!ProviderInterface)
        return false;

    if (GetClass()->ImplementsInterface(ProviderInterface))
        return true;

    auto actor = GetOwner();
    if (!actor)
        return false;

    if (actor->GetClass()->ImplementsInterface(ProviderInterface))
        return true;

    for (auto comp : actor->GetComponents())
    {
        if (comp && comp->GetClass()->ImplementsInterface(ProviderInterface))
            return true;
    }

    return false;
}

bool UArticyFlowPlayer::OnTick(float DeltaTime)
{
    bool bPlayedBranch = false;

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

            // update nodes visited - connections are exported without an id, so they are not tracked
            auto* GVs = GetGVs();
            auto* primitive = Cast<UArticyPrimitive>(node.GetObject());
            if (GVs && primitive && !primitive->GetId().IsNull())
            {
                GVs->IncrementSeenCounter(Cast<IArticyFlowObject>(node.GetObject()));
            }
        }

        Cursor = Branch.Path.Last();
        bPlayedBranch = true;
        UpdateAvailableBranches();
    }

    // Branches a GV change made available or unavailable are only picked up by re-exploring.
    // Skipped right after a branch was played, which already explored from the new position.
    if (AdvanceRecalculationTimer(DeltaTime) && !bPlayedBranch && Cursor)
    {
        RefreshBranches();
    }

    return true;
}

//---------------------------------------------------------------------------//

/**
 * Updates the available branches from the current cursor position.
 *
 * @param Startup Whether this is a startup update.
 * @param bPause Whether OnPlayerPaused should be broadcast along with the new branches.
 */
void UArticyFlowPlayer::UpdateAvailableBranchesInternal(bool Startup, bool bPause)
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

        if (SortBranches != nullptr)
        {
            AvailableBranches.Sort(SortBranches);
        }

        // If we're just starting up, check if we should fast-forward
        if (Startup && FastForwardToPause())
        {
            //fast-forwarding will call UpdateAvailableBranches again, can abort here
            return;
        }

        if (bPause && !bPausedWithoutCallbacks && !OnPlayerPaused.IsBound() && !OnBranchesUpdated.IsBound())
        {
            bPausedWithoutCallbacks = true;
            UE_LOG(LogArticyRuntime, Warning, TEXT("ArticyFlowPlayer on %s paused, but nothing is bound to OnPlayerPaused or OnBranchesUpdated."), *GetNameSafe(GetOwner()))
        }

        //broadcast and return result
        if (bPause)
            OnPlayerPaused.Broadcast(Cursor);
        OnBranchesUpdated.Broadcast(AvailableBranches);
    }
}

/**
 * Sets the cursor to the start node.
 */
void UArticyFlowPlayer::SetCursorToStartNode()
{
    // This ensure Flowplayer construction without throwing
    // error message when setup in Actor construction with C++
    if (StartOn.GetId().IsNull())
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

#undef LOCTEXT_NAMESPACE
