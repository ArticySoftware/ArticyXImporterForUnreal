//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyReflectable.h"
#include "ArticyHelpers.h"
#include "UObject/Interface.h"
#include "ArticyObjectWith_Base.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWith_Base : public UArticyReflectable { GENERATED_BODY() };

/**
 * All objects that have a property called 'Color' implement this interface.
 * The interfaces deriving from this one usually have three methods:
 * 
 * - UFUNCTION getter returning a non-const reference
 * - const getter returning a const reference
 * - UFUNCTION setter returning a non-const reference (as returning a const-reference
 *														does not work for blueprints!)
 *													
 * If IDs are involved, there are getters/setters working on the pointed-to
 * objects for convenience.
 */
class IArticyObjectWith_Base : public IArticyReflectable
{
	GENERATED_BODY()

protected:

	template<typename PropType>
	PropType& GetProperty(const FName& PropName)
	{
		auto prop = GetPropPtr<PropType>(PropName);

		if(ensure(prop))
			return *prop;
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot get property %s from object %s!"),
				   *PropName.ToString(), GetReflectedObject() ? *GetReflectedObject()->GetName() : TEXT("(nullptr)"));
		}

		static PropType Empty;
		return Empty;
	}

	/**
	 * The reflected property behind a generated string property, which is an FString for an
	 * ArticyString and an FText for an ArticyMultiLanguageString.
	 */
	FProperty* GetStringProperty(const FName& PropName)
	{
		FProperty* Prop = IArticyReflectable::GetProperty(PropName);

		if (ArticyHelpers::GetTextPropertyKind(Prop) == ArticyHelpers::EArticyTextPropertyKind::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot get string property %s from object %s!"),
				   *PropName.ToString(), GetReflectedObject() ? *GetReflectedObject()->GetName() : TEXT("(nullptr)"));
			return nullptr;
		}

		return Prop;
	}

	/** Display text of a string property: localized for an FText, text extension only for an FString. */
	FText GetStringText(const FName& PropName, const FText* BackupText = nullptr)
	{
		return ArticyHelpers::GetTextPropertyValue(GetReflectedObject(), GetStringProperty(PropName), true, BackupText);
	}

	/** Raw value of a string property: the string table key of an FText, or the FString itself. */
	FString GetStringKey(const FName& PropName)
	{
		return ArticyHelpers::GetTextPropertyKey(GetReflectedObject(), GetStringProperty(PropName));
	}

	/** Writes Text into a string property of either kind and returns what is stored now. */
	FText SetStringText(const FName& PropName, const FText& Text)
	{
		ArticyHelpers::SetTextPropertyValue(GetReflectedObject(), GetStringProperty(PropName), Text);
		return FText::FromString(GetStringKey(PropName));
	}

};
