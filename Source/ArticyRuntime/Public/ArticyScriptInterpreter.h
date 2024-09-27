//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyGlobalVariables.h"
#include "SWarningOrErrorBox.h"

#include "ArticyScriptInterpreter.generated.h"

USTRUCT()
struct FScriptExpressionParser
{
	GENERATED_USTRUCT_BODY()

	static bool Evaluate(const FString& Expression, UArticyGlobalVariables* Globals)
{
	if (Expression.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty expression."));
		return false;
	}

	// Handle compound logical expressions
	if (Expression.Contains(TEXT("&&")))
	{
		TArray<FString> SubExpressions;
		Expression.ParseIntoArray(SubExpressions, TEXT("&&"), true);

		for (const FString& SubExpression : SubExpressions)
		{
			if (!Evaluate(SubExpression.TrimStartAndEnd(), Globals))
			{
				return false; // If any sub-expression is false, the whole condition is false
			}
		}
		return true; // All sub-expressions are true
	}
	else if (Expression.Contains(TEXT("||")))
	{
		TArray<FString> SubExpressions;
		Expression.ParseIntoArray(SubExpressions, TEXT("||"), true);

		for (const FString& SubExpression : SubExpressions)
		{
			if (Evaluate(SubExpression.TrimStartAndEnd(), Globals))
			{
				return true; // If any sub-expression is true, the whole condition is true
			}
		}
		return false; // All sub-expressions are false
	}

	// Tokenize the simple expression (e.g., "door_open == true")
	TArray<FString> Tokens;
	Expression.ParseIntoArray(Tokens, TEXT(" "), true);

	if (Tokens.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid expression: %s"), *Expression);
		return false;
	}

	FArticyGvName VariableName = FName(Tokens[0]);
	FString Operator = Tokens[1];
	FString Value = Tokens[2];

	bool bSuccess = false;

		// TODO: Generalize this
	// Attempt to retrieve the variable dynamically
	if (Value == TEXT("true") || Value == TEXT("false"))
	{
		// Handle boolean comparison
		bool BoolValue = Globals->GetBoolVariable(VariableName, bSuccess);
		if (bSuccess)
		{
			bool ComparisonValue = (Value == TEXT("true"));
			if (Operator == TEXT("==")) return BoolValue == ComparisonValue;
			if (Operator == TEXT("!=")) return BoolValue != ComparisonValue;
			UE_LOG(LogTemp, Warning, TEXT("Unsupported operator '%s' for boolean comparison."), *Operator);
			return false;
		}
	}
	else if (Value.IsNumeric())
	{
		// Handle integer comparison
		int IntValue = Globals->GetBoolVariable(VariableName, bSuccess);
		if (bSuccess)
		{
			int ComparisonValue = FCString::Atoi(*Value);
			if (Operator == TEXT("==")) return IntValue == ComparisonValue;
			if (Operator == TEXT("!=")) return IntValue != ComparisonValue;
			if (Operator == TEXT(">")) return IntValue > ComparisonValue;
			if (Operator == TEXT("<")) return IntValue < ComparisonValue;
			if (Operator == TEXT(">=")) return IntValue >= ComparisonValue;
			if (Operator == TEXT("<=")) return IntValue <= ComparisonValue;
			UE_LOG(LogTemp, Warning, TEXT("Unsupported operator '%s' for integer comparison."), *Operator);
			return false;
		}
	}
	else
	{
		// Handle string comparison
		FString StringValue = Globals->GetStringVariable(VariableName, bSuccess);
		if (bSuccess)
		{
			if (Operator == TEXT("==")) return StringValue == Value;
			if (Operator == TEXT("!=")) return StringValue != Value;
			UE_LOG(LogTemp, Warning, TEXT("Unsupported operator '%s' for string comparison."), *Operator);
			return false;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Variable '%s' not found or unsupported type."), *Tokens[0]);
	return false;
}
};

USTRUCT()
struct FScriptInstructionExecutor
{
	GENERATED_USTRUCT_BODY()

	static void Execute(const FString& Instruction, UArticyGlobalVariables* Globals)
	{
		// Tokenize the instruction (example: "door_open = true")
		TArray<FString> Tokens;
		Instruction.ParseIntoArray(Tokens, TEXT(" "), true);

		if (Tokens.Num() < 3)
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid instruction: %s"), *Instruction);
			return;
		}

		FArticyGvName VariableName = FName(Tokens[0]);
		FString Operator = Tokens[1];
		FString Value = Tokens[2];

		if (Operator == TEXT("="))
		{
			if (Value.ToBool())
			{
				Globals->SetBoolVariable(VariableName, Value.ToBool());
			}
			else if (FCString::IsNumeric(*Value))
			{
				Globals->SetIntVariable(VariableName, FCString::Atoi(*Value));
			}
			else
			{
				Globals->SetStringVariable(VariableName, Value);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unsupported operator in instruction: %s"), *Operator);
		}
	}
};

/**
 * Articy script interpreter for Expresso scripts
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyScriptInterpreter : public UObject
{
	GENERATED_BODY()

public:
	static bool EvaluateCondition(const FString& Condition, UArticyGlobalVariables* Globals)
	{
		return FScriptExpressionParser::Evaluate(Condition, Globals);
	}

	static void ExecuteInstruction(const FString& Instruction, UArticyGlobalVariables* Globals)
	{
		FScriptInstructionExecutor::Execute(Instruction, Globals);
	}
};
