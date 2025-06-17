//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObjectWith_Base.h"
#include "ArticyObjectWithText.h"
#include "ArticyTextExtension.h"
#include "ArticyObjectWithMenuText.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWithMenuText : public UArticyObjectWith_Base { GENERATED_BODY() };

/**
 * All objects that have a property called 'MenuText' implement this interface.
 */
class IArticyObjectWithMenuText : public IArticyObjectWith_Base
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithMenuText")
	virtual FText GetMenuText()
	{
		static const auto& PropName = FName("MenuText");
		static const auto& BackupText = FText::FromString("...");
		return GetStringText(Cast<UObject>(this), PropName, &BackupText);
	}

	virtual FText GetMenuText() const
	{
		return const_cast<IArticyObjectWithMenuText*>(this)->GetMenuText();
	}
	
	//---------------------------------------------------------------------------//

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithMenuText")
	virtual FText& SetMenuText(UPARAM(ref) const FText& MenuText)
	{
		static const auto& PropName = FName("MenuText");
		return GetProperty<FText>(PropName) = MenuText;
	}
};
