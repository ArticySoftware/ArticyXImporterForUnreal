//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyHierarchyNode.h"
#include "ArticyDatabase.h"
#include "ArticyObject.h"

/**
 * Tries to return the underlying object through the database owning this node's hierarchy.
 * @param CloneId The clone ID of the object.
 * @param CastTo The class to cast the object to.
 * @return The object, or nullptr if it is not exported, not loaded, or not of class CastTo.
 */
UArticyObject* UArticyHierarchyNode::GetObject(int32 CloneId, TSubclassOf<UArticyObject> CastTo) const
{
	const UArticyDatabase* Database = GetTypedOuter<UArticyDatabase>();
	if (!Database)
		return nullptr;

	UArticyObject* Object = Database->GetObject(Id, CloneId);
	if (Object && CastTo && !Object->IsA(CastTo))
		return nullptr;

	return Object;
}
