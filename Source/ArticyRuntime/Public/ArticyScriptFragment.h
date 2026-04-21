//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPausableType.h"
#include "Interfaces/ArticyNode.h"
#include "Interfaces/ArticyConditionProvider.h"
#include "Interfaces/ArticyInstructionProvider.h"
#include "ArticyScriptFragment.generated.h"

/**
 * Base class for the script fragments (ArticyScriptCondition or ArticyScriptInstruction).
 *
 * This class represents a fragment of a script, which may contain an expression used for conditions
 * or instructions within the Articy runtime environment.
 */
UCLASS(BlueprintType, abstract)
class ARTICYRUNTIME_API UArticyScriptFragment : public UArticyPrimitive
{
    GENERATED_BODY()
public:
    /**
     * Retrieves the expression associated with this script fragment.
     *
     * @return A constant reference to the expression string.
     */
    const FString& GetExpression() const { return Expression; }
protected:

    /**
     * The expression associated with this script fragment.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
    FString Expression = "";

    /**
     * Returns a cached hash of the expression.
     *
     * @return The cached hash of the expression.
     */
    int GetExpressionHash() const;

    template<typename Type, typename PropType>
    friend struct ArticyObjectTypeInfo;

    /**
     * Initializes the script fragment from a JSON value.
     *
     * @param Json A shared pointer to the JSON value to initialize from.
     */
    void InitFromJson(TSharedPtr<FJsonValue> Json) override;

private:

    /**
     * Cached hash of the expression to optimize performance.
     */
    mutable int CachedExpressionHash;
};

/** -------------------------------------------------------------------------------- */

/**
 * A script condition that evaluates an expression.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyScriptCondition : public UArticyScriptFragment, public IArticyConditionProvider
{
    GENERATED_BODY()

public:

    /**
     * Evaluates the script condition.
     *
     * @param GV A pointer to the global variables used for evaluation.
     * @param MethodProvider A pointer to the method provider object.
     * @return True if the condition evaluates to true, false otherwise.
     */
    bool Evaluate(class UArticyGlobalVariables* GV = nullptr, class UObject* MethodProvider = nullptr) override;
};

/**
 * A condition node, which has a script condition.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyCondition : public UArticyNode, public IArticyConditionProvider
{
    GENERATED_BODY()

public:
    /**
     * Retrieves the script condition associated with this condition node.
     *
     * @return A pointer to the script condition.
     */
    UFUNCTION(BlueprintCallable, Category = "Condition")
    UArticyScriptCondition* GetCondition() const;

    /**
     * Retrieves the type of the node as a condition.
     *
     * @return The type of the node.
     */
    EArticyPausableType GetType() override { return EArticyPausableType::Condition; }

    /**
     * Evaluates the condition node.
     *
     * @param GV A pointer to the global variables used for evaluation.
     * @param MethodProvider A pointer to the method provider object.
     * @return True if the condition evaluates to true, false otherwise.
     */
    bool Evaluate(class UArticyGlobalVariables* GV = nullptr, class UObject* MethodProvider = nullptr) override;

    /**
     * Explores the condition node and appends the resulting branches to the output array.
     *
     * @param Player A pointer to the flow player handling the exploration.
     * @param OutBranches An array to which the resulting branches are appended.
     * @param Depth The current depth of exploration.
     */
    void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override;

private:

    /**
     * The script condition associated with this condition node.
     */
    UPROPERTY(transient)
    mutable UArticyScriptCondition* Condition = nullptr;
};

/** -------------------------------------------------------------------------------- */

/**
 * A script instruction that executes an expression.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyScriptInstruction : public UArticyScriptFragment, public IArticyInstructionProvider
{
    GENERATED_BODY()

public:

    /**
     * Executes the script instruction.
     *
     * @param GV A pointer to the global variables used for execution.
     * @param MethodProvider A pointer to the method provider object.
     */
    void Execute(class UArticyGlobalVariables* GV = nullptr, class UObject* MethodProvider = nullptr) override;
};

/**
 * An instruction node, which has a script instruction.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyInstruction : public UArticyNode, public IArticyInstructionProvider
{
    GENERATED_BODY()

public:
    /**
     * Retrieves the script instruction associated with this instruction node.
     *
     * @return A pointer to the script instruction.
     */
    UFUNCTION(BlueprintCallable, Category = "Instruction")
    UArticyScriptInstruction* GetInstruction() const;

    /**
     * Retrieves the type of the node as an instruction.
     *
     * @return The type of the node.
     */
    EArticyPausableType GetType() override { return EArticyPausableType::Instruction; }

    /**
     * Executes the instruction node.
     *
     * @param GV A pointer to the global variables used for execution.
     * @param MethodProvider A pointer to the method provider object.
     */
    void Execute(class UArticyGlobalVariables* GV = nullptr, class UObject* MethodProvider = nullptr) override;

    /**
     * Explores the instruction node and appends the resulting branches to the output array.
     *
     * @param Player A pointer to the flow player handling the exploration.
     * @param OutBranches An array to which the resulting branches are appended.
     * @param Depth The current depth of exploration.
     */
    void Explore(UArticyFlowPlayer* Player, TArray<FArticyBranch>& OutBranches, const uint32& Depth) override;

private:

    /**
     * The script instruction associated with this instruction node.
     */
    UPROPERTY(transient)
    mutable UArticyScriptInstruction* Instruction = nullptr;
};
