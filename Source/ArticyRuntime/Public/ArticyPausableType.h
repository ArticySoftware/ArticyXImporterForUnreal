//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPausableType.generated.h"

/**
* Enum representing the various types of Articy flow nodes that can be paused on.
 */
UENUM(BlueprintType, meta = (Bitflags))
enum class EArticyPausableType : uint8
{
	FlowFragment,
	Dialogue,
	DialogueFragment,
	Hub,
	Jump,
	Condition,
	Instruction,
	Pin
};
ENUM_CLASS_FLAGS(EArticyPausableType);
