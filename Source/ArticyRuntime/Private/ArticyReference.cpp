//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyReference.h"

UArticyReference::UArticyReference()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * Checks if the current Articy reference is valid.
 *
 * @return True if the reference ID is not zero, otherwise false.
 */
bool UArticyReference::IsValid()
{
	return Reference.GetId().Get() != 0;
}

/**
 * Retrieves the ArticyPrimitive object referenced by this component.
 *
 * @param WorldContext The context used to obtain the Articy object.
 * @return A pointer to the ArticyPrimitive object, or nullptr if not found.
 */
UArticyPrimitive* UArticyReference::GetObject(const UObject* WorldContext)
{
	return Reference.GetObject<UArticyPrimitive>(WorldContext);
}

/**
 * Sets the reference to an Articy object. Updates the internal reference to match the provided object.
 *
 * @param Object The Articy object to reference.
 */
void UArticyReference::SetReference(UArticyObject* Object)
{
	Reference.SetReference(Object);
}
