//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyScriptFragment.h"
#include "ArticyExpressoScripts.h"

/**
 * Computes and returns a hash of the expression.
 * The hash is cached for performance optimization.
 *
 * @return The hash value of the expression.
 */
int UArticyScriptFragment::GetExpressionHash() const
{
    if (!CachedExpressionHash)
        CachedExpressionHash = GetTypeHash(Expression);

    return CachedExpressionHash;
}

/**
 * Initializes the script fragment from a JSON value.
 * This method parses the JSON value and sets the expression if the JSON is valid.
 *
 * @param Json A shared pointer to the JSON value to initialize from.
 */
void UArticyScriptFragment::InitFromJson(TSharedPtr<FJsonValue> Json)
{
    Super::InitFromJson(Json);

    if (!Json.IsValid() || !ensure(Json->Type == EJson::String))
        return;

    // Import the fragment expression
    auto exp = Json->AsString();
    if (!exp.IsEmpty())
        Expression = *exp;
}

//---------------------------------------------------------------------------//

/**
 * Evaluates the script condition.
 * This method uses the Expresso instance to evaluate the condition expression hash.
 *
 * @param GV A pointer to the global variables used for evaluation.
 * @param MethodProvider A pointer to the method provider object.
 * @return True if the condition evaluates to true, false otherwise.
 */
bool UArticyScriptCondition::Evaluate(class UArticyGlobalVariables* GV, class UObject* MethodProvider)
{
    auto db = UArticyDatabase::Get(this);
    return db->GetExpressoInstance()->Evaluate(GetExpressionHash(), GV ? GV : db->GetGVs(), MethodProvider);
}

/**
 * Evaluates the condition node.
 * If a script condition is set, it delegates the evaluation to the script condition's Evaluate method.
 *
 * @param GV A pointer to the global variables used for evaluation.
 * @param MethodProvider A pointer to the method provider object.
 * @return True if the condition evaluates to true, false otherwise.
 */
bool UArticyCondition::Evaluate(UArticyGlobalVariables* GV, UObject* MethodProvider)
{
    return !GetCondition() || GetCondition()->Evaluate(GV, MethodProvider);
}

/**
 * Explores the condition node and appends the resulting branches to the output array.
 * The exploration follows the output pins based on the condition evaluation result.
 *
 * @param Player A pointer to the flow player handling the exploration.
 * @param OutBranches An array to which the resulting branches are appended.
 * @param Depth The current depth of exploration.
 */
void UArticyCondition::Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth)
{
    auto pins = GetOutputPinsPtr();
    if (!pins || !ensure(pins->Num() == 2))
    {
        // Conditions MUST have 2 output pins!
        // Continue on output pins, will also handle the case where pins are not valid
        Super::Explore(Player, OutBranches, Depth);
        return;
    }

    if (Evaluate(Player->GetGVs(), Player->GetMethodsProvider()))
        OutBranches.Append(Player->Explore((*pins)[0], false, Depth + 1)); // TRUE
    else
        OutBranches.Append(Player->Explore((*pins)[1], false, Depth + 1)); // FALSE
}

//---------------------------------------------------------------------------//

/**
 * Executes the script instruction.
 * This method uses the Expresso instance to execute the instruction based on the expression hash.
 *
 * @param GV A pointer to the global variables used for execution.
 * @param MethodProvider A pointer to the method provider object.
 */
void UArticyScriptInstruction::Execute(class UArticyGlobalVariables* GV, class UObject* MethodProvider)
{
    auto db = UArticyDatabase::Get(this);
    db->GetExpressoInstance()->Execute(GetExpressionHash(), GV ? GV : db->GetGVs(), MethodProvider);
}

/**
 * Retrieves the script condition associated with this condition node.
 * If the condition has not been previously loaded, it is retrieved from the property.
 *
 * @return A pointer to the script condition.
 */
UArticyScriptCondition* UArticyCondition::GetCondition() const
{
    if (!Condition)
    {
        FName Expression = TEXT("Expression");
        Condition = static_cast<const IArticyReflectable*>(static_cast<const UArticyBaseObject*>(this))->GetProp<UArticyScriptCondition*>(Expression);
    }

    return Condition;
}

/**
 * Retrieves the script instruction associated with this instruction node.
 * If the instruction has not been previously loaded, it is retrieved from the property.
 *
 * @return A pointer to the script instruction.
 */
UArticyScriptInstruction* UArticyInstruction::GetInstruction() const
{
    if (!Instruction)
    {
        FName Expression = TEXT("Expression");
        Instruction = static_cast<const IArticyReflectable*>(static_cast<const UArticyBaseObject*>(this))->GetProp<UArticyScriptInstruction*>(Expression);
    }

    return Instruction;
}

/**
 * Executes the instruction node.
 * If a script instruction is associated with the node, it delegates the execution to the script instruction.
 *
 * @param GV A pointer to the global variables used for execution.
 * @param MethodProvider A pointer to the method provider object.
 */
void UArticyInstruction::Execute(UArticyGlobalVariables* GV, UObject* MethodProvider)
{
    if (GetInstruction())
        GetInstruction()->Execute(GV, MethodProvider);
}

/**
 * Explores the instruction node and appends the resulting branches to the output array.
 * The exploration continues on the output pins after executing the instruction.
 *
 * @param Player A pointer to the flow player handling the exploration.
 * @param OutBranches An array to which the resulting branches are appended.
 * @param Depth The current depth of exploration.
 */
void UArticyInstruction::Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth)
{
    Execute(Player->GetGVs(), Player->GetMethodsProvider());

    // Continue on output pins
    Super::Explore(Player, OutBranches, Depth);
}
