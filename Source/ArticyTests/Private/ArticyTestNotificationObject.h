//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "ArticyChangedProperty.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "ArticyTestNotificationObject.generated.h"

/**
 * @brief Stand-in for a generated feature, whose properties are the template properties.
 */
UCLASS()
class UArticyTestNotifyFeature : public UArticyBaseFeature
{
	GENERATED_BODY()

public:

	UPROPERTY()
	int32 MoraleValue = 0;

	UPROPERTY()
	FString Mood;
};

/**
 * @brief Stand-in for a generated base type like Entity, with general properties only.
 */
UCLASS()
class UArticyTestNotifyEntity : public UArticyObject, public IArticyObjectWithDisplayName
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	int32 Level = 0;
};

/**
 * @brief Stand-in for a generated template type, which is based on the entity and adds a feature.
 */
UCLASS()
class UArticyTestNotifyCharacter : public UArticyTestNotifyEntity
{
	GENERATED_BODY()

public:

	/** Creates the feature as a subobject, like the importer does. */
	void CreateFeature() { Morale = NewObject<UArticyTestNotifyFeature>(this); }

	UPROPERTY()
	UArticyTestNotifyFeature* Morale = nullptr;

	UPROPERTY()
	int32 Courage = 0;
};

/**
 * @brief Object of a type unrelated to the entity, to tell type listeners apart.
 */
UCLASS()
class UArticyTestNotifyLocation : public UArticyObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	int32 Size = 0;
};

/**
 * @brief Records the changes it is notified about, to bind as a Blueprint listener.
 */
UCLASS()
class UArticyTestNotificationListener : public UObject
{
	GENERATED_BODY()

public:

	/** The properties reported, in order. */
	TArray<FString> Properties;

	/** The object reported by the last change. */
	UPROPERTY()
	UArticyBaseObject* LastObject = nullptr;

	/** Records a reported change. */
	UFUNCTION()
	void HandleChanged(const FArticyChangedProperty& ChangedProperty)
	{
		Properties.Add(ChangedProperty.Property.ToString());
		LastObject = ChangedProperty.ObjectReference;
	}
};
