//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyPausableType.generated.h"

/**
* Enum representing the various types of Articy flow nodes that can be paused on.
* The values are bit indices: a set of types is stored as a bitmask of 1 << value,
* which is what UArticyFlowPlayer::PauseOn holds.
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
	Pin,
	/** A connection between two pins. */
	Connection
};

/**
 * Returns the PauseOn bitmask flag of a pausable type.
 *
 * @param Type The pausable type to convert.
 * @return The single bit representing Type in a PauseOn bitmask.
 */
FORCEINLINE int32 ArticyPauseOnFlag(EArticyPausableType Type)
{
	return 1 << static_cast<uint8>(Type);
}
