//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObjectWith_Base.h"
#include "ArticyObjectWithVertices.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWithVertices : public UArticyObjectWith_Base { GENERATED_BODY() };

/**
 * All objects that have a property called 'Vertices' implement this interface.
 */
class IArticyObjectWithVertices : public IArticyObjectWith_Base
{
	GENERATED_BODY()

public:
	
	virtual TArray<FVector2D>& GetVertices()
	{
		static const auto& PropName = FName("Vertices");
		return GetProperty<TArray<FVector2D>>(PropName);
	}

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithVertices")
	virtual const TArray<FVector2D>& GetVertices() const
	{
		return const_cast<IArticyObjectWithVertices*>(this)->GetVertices();
	}
	
	//---------------------------------------------------------------------------//

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithVertices")
	virtual TArray<FVector2D>& SetVertices(UPARAM(ref) const TArray<FVector2D>& Vertices)
	{
		return GetVertices() = Vertices;
	}
};
