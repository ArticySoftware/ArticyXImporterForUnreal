//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObjectWith_Base.h"
#include "ArticyObjectWithPosition.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWithPosition : public UArticyObjectWith_Base { GENERATED_BODY() };

/**
 * All objects that have a property called 'Position' implement this interface.
 */
class IArticyObjectWithPosition : public IArticyObjectWith_Base
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithPosition")
	virtual FVector2D& GetPosition()
	{
		static const auto& PropName = FName("Position");
		return GetProperty<FVector2D>(PropName);
	}

	virtual const FVector2D& GetPosition() const
	{
		return const_cast<IArticyObjectWithPosition*>(this)->GetPosition();
	}
	
	//---------------------------------------------------------------------------//

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithPosition")
	virtual FVector2D& SetPosition(UPARAM(ref) const FVector2D& Position)
	{
		return GetPosition() = Position;
	}
};
