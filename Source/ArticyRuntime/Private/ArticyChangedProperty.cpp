//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyChangedProperty.h"
#include "ArticyBaseObject.h"

void FArticyChangedProperty::SetObjectReference(IArticyReflectable* object)
{
	if (UArticyBaseObject* baseObject = Cast<UArticyBaseObject>(object))
	{
		ObjectReference = baseObject;
	}
}
