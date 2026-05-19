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
	/** A flow fragment node. */
	FlowFragment,
	/** A dialogue node. */
	Dialogue,
	/** A dialogue fragment node. */
	DialogueFragment,
	/** A hub node. */
	Hub,
	/** A jump node. */
	Jump,
	/** A condition node. */
	Condition,
	/** An instruction node. */
	Instruction,
	/** A flow pin (input or output). */
	Pin
};
ENUM_CLASS_FLAGS(EArticyPausableType);
