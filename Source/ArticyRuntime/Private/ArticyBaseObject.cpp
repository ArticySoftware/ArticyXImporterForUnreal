//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
 
//


#include "ArticyBaseObject.h"
#include "ArticyPrimitive.h"
#include "ArticyTypeSystem.h"

UArticyPrimitive* UArticyBaseObject::GetSubobject(FArticyId Id) const
{
	auto obj = Subobjects.Find(Id);
	return obj ? *obj : nullptr;
}

void UArticyBaseObject::AddSubobject(UArticyPrimitive* Obj)
{
	Subobjects.Add(Obj->GetId(), Obj);
}

FArticyType UArticyBaseObject::GetArticyType() const
{
	return ArticyType;
}