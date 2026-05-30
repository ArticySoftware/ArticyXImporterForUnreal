//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyBaseObject.h"
#include "ArticyPrimitive.h"
#include "ArticyTypeSystem.h"
#include "ArticyHelpers.h"

/**
 * Retrieves a subobject of this Articy object using its unique identifier.
 *
 * @param Id The unique identifier of the subobject to retrieve.
 * @return A pointer to the UArticyPrimitive subobject if found, otherwise nullptr.
 */
UArticyPrimitive* UArticyBaseObject::GetSubobject(FArticyId Id) const
{
	auto obj = Subobjects.Find(Id);
	return obj ? *obj : nullptr;
}

/**
 * Adds a subobject to this Articy object.
 *
 * @param Obj The subobject to add.
 */
void UArticyBaseObject::AddSubobject(UArticyPrimitive* Obj)
{
	Subobjects.Add(Obj->GetId(), Obj);
}

/**
 * Gets the Articy type of this object.
 *
 * @return The FArticyType associated with this object.
 */
FArticyType UArticyBaseObject::GetArticyType() const
{
	return ArticyType;
}

/**
 * Retrieves a localized text property for this Articy object.
 *
 * @param Property The text property to localize.
 * @return A localized FText object.
 */
FText UArticyBaseObject::GetPropertyText(const FText Property)
{
	return ArticyHelpers::LocalizeString(this, Property, true, &Property);
}
