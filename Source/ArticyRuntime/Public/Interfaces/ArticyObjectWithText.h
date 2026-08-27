//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyObjectWith_Base.h"
#include "UObject/TextProperty.h"
#include "Engine/Engine.h"
#include "Internationalization/StringTable.h"
#include "ArticyAsset.h"
#include "ArticyDatabase.h"
#include "ArticyTextExtension.h"
#include "UObject/Interface.h"
#include "ArticyObjectWithText.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UArticyObjectWithText : public UArticyObjectWith_Base { GENERATED_BODY() };

/**
 * All objects that have a property called 'Text' implement this interface.
 */
class IArticyObjectWithText : public IArticyObjectWith_Base
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithText")
	virtual FText GetText()
	{
		static const auto& PropName = FName("Text");
		return GetStringText(PropName);
	}

	virtual FText GetText() const
	{
		return const_cast<IArticyObjectWithText*>(this)->GetText();
	}
	
	//---------------------------------------------------------------------------//

	UFUNCTION(BlueprintCallable, Category="ArticyObjectWithText")
	virtual FText SetText(UPARAM(ref) const FText& Text)
	{
		static const auto& PropName = FName("Text");
		return SetStringText(PropName, Text);
	}

	UFUNCTION(BlueprintCallable, Category = "ArticyObjectWithText")
	virtual USoundWave* GetVOAsset(UObject* WorldContext)
	{
		static const auto& PropName = FName("Text");

		// VO assets are exported alongside the localized texts, so a plain ArticyString never has one
		if (ArticyHelpers::GetTextPropertyKind(GetStringProperty(PropName)) != ArticyHelpers::EArticyTextPropertyKind::LocalizedText)
		{
			return nullptr;
		}

		const FString Key = GetStringKey(PropName);
		const FText MissingEntry = FText::FromString("<MISSING STRING TABLE ENTRY>");
		FArticyId AssetId;

		const FName TableName = TEXT("ARTICY");

		// Find the table
		FStringTableConstPtr TablePtr = FStringTableRegistry::Get().FindStringTable(TableName);
		if (!TablePtr.IsValid())
		{
			return nullptr;
		}

		// Find the entry
		const FStringTable* Table = TablePtr.Get();
		FStringTableEntryConstPtr EntryPtr = Table->FindEntry(FTextKey(Key + ".VOAsset"));
		if (!EntryPtr.IsValid())
		{
			return nullptr;
		}

		const FStringTableEntry* TableEntry = EntryPtr.Get();
		FText SourceString = FText::FromString(TableEntry->GetSourceString());

		if (!SourceString.IsEmpty() && !SourceString.EqualTo(MissingEntry))
		{
			AssetId = FArticyId{ ResolveText(WorldContext, &SourceString).ToString() };
		}
		else
		{
			const auto& AssetString = FText::FromString(Key + ".VOAsset");
			AssetId = FArticyId{ ResolveText(WorldContext, &AssetString).ToString() };
		}

		const UArticyDatabase* Database = UArticyDatabase::Get(WorldContext);
		if (!Database)
		{
			return nullptr;
		}
		const UArticyObject* AssetObject = Database->GetObject(AssetId);
		if (!AssetObject)
		{
			return nullptr;
		}
		return (Cast<UArticyAsset>(AssetObject))->LoadAsSoundWave();
	}

	virtual FText ResolveText(UObject* Outer, const FText* SourceText)
	{
		return ArticyHelpers::ResolveText(Outer, SourceText);
	}
};
