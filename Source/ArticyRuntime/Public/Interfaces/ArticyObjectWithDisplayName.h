//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObjectWith_Base.h"
#include "UObject/TextProperty.h"
#include "ArticyBaseTypes.h"
#include "ArticyObjectWithDisplayName.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWithDisplayName : public UArticyObjectWith_Base { GENERATED_BODY() };

/**
 * All objects that have a property called 'DisplayName' implement this interface.
 */
class IArticyObjectWithDisplayName : public IArticyObjectWith_Base
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithDisplayName")
	virtual FText GetDisplayName()
	{
		static const auto& PropName = FName("DisplayName");
		return GetStringText(Cast<UObject>(this), PropName);
	}

	virtual const FText GetDisplayName() const
	{
		return const_cast<IArticyObjectWithDisplayName*>(this)->GetDisplayName();
	}
	
	//---------------------------------------------------------------------------//

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithDisplayName")
	virtual FText& SetDisplayName(UPARAM(ref) const FText& DisplayName)
	{
		static const auto& PropName = FName("DisplayName");
		return GetProperty<FText>(PropName) = DisplayName;
	}
};
