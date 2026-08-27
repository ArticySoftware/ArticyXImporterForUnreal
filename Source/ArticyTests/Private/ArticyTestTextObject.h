//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithText.h"
#include "Interfaces/ArticyObjectWithMenuText.h"
#include "Interfaces/ArticyObjectWithStageDirections.h"
#include "ArticyTestTextObject.generated.h"

/**
 * @brief Stand-in for a generated class mixing both articy text kinds.
 *
 * DisplayName and StageDirections are FStrings (ArticyString), Text and MenuText are FTexts
 * (ArticyMultiLanguageString), as a generated Hub or DialogueFragment has them.
 */
UCLASS()
class UArticyTestTextObject : public UArticyObject,
	public IArticyObjectWithDisplayName,
	public IArticyObjectWithText,
	public IArticyObjectWithMenuText,
	public IArticyObjectWithStageDirections
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FText Text;

	UPROPERTY()
	FText MenuText;

	UPROPERTY()
	FString StageDirections;

	/** Not a text property at all. */
	UPROPERTY()
	int32 Number = 0;
};

/**
 * @brief Object whose Text is a plain ArticyString, like a Hub or an Instruction.
 */
UCLASS()
class UArticyTestPlainTextObject : public UArticyObject, public IArticyObjectWithText
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FString Text;
};
