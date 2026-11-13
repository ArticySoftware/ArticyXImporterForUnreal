//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Interfaces/ArticyReflectable.h"
#include "ArticyBaseObject.h"
#include "ArticyObjectNotificationManager.h"

void IArticyReflectable::NotifyPropertyChanged(FName Property)
{
	FArticyChangedProperty ChangedProperty;
	ChangedProperty.Property = Property;
	ChangedProperty.SetObjectReference(this);

	ReportChanged.Broadcast(ChangedProperty);

	// Only set for articy objects, not for the variable sets
	if (ChangedProperty.ObjectReference)
		UArticyObjectNotificationManager::ReportObjectChanged(ChangedProperty.ObjectReference, Property);
}
