//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyType.h"
#include "ArticyTextExtension.h"
#include "Internationalization/StringTableRegistry.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "Internationalization/StringTableCore.h"
#include "ArticyLocalizerSystem.generated.h"

UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyLocalizerSystem : public UObject
{
	GENERATED_BODY()

public:
	static UArticyLocalizerSystem* Get()
	{
		static TWeakObjectPtr<UArticyLocalizerSystem> ArticyLocalizerSystem;
		if (ArticyLocalizerSystem.IsValid())
		{
			return ArticyLocalizerSystem.Get();
		}

		UClass* ParentClass = UArticyLocalizerSystem::StaticClass();

		// Iterate over all classes
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			// Check if the class is derived from the parent class
			if (Class->IsChildOf(ParentClass) && Class != ParentClass)
			{
				// Create an instance of the found subclass
				UArticyLocalizerSystem* NewLocalizerSystem = NewObject<UArticyLocalizerSystem>(GetTransientPackage(), Class);

				// Assign the instance to the TWeakObjectPtr
				ArticyLocalizerSystem = NewLocalizerSystem;

				return ArticyLocalizerSystem.Get();
			}
		}

		// Handle case where no subclass is found
		return nullptr;
	}

	virtual void Reload() {};

	inline FText ResolveText(UObject* Outer, const FText* SourceText)
	{
		return UArticyTextExtension::Get()->Resolve(Outer, SourceText);
	}

	inline FText LocalizeString(UObject* Outer, const FText& Key, bool ResolveTextExtension = true, const FText* BackupText = nullptr)
	{
		if (!bDataLoaded)
		{
			Reload();
		}

		const FText MissingEntry = FText::FromString("<MISSING STRING TABLE ENTRY>");

		// Default to key
		FText SourceString = Key;

		// Look up entry in specified string table
		TOptional<FString> TableName = FTextInspector::GetNamespace(Key);
		if (!TableName.IsSet())
		{
			TableName = TEXT("ARTICY");
		}

		// Find the table
		FStringTableConstPtr TablePtr = FStringTableRegistry::Get().FindStringTable(FName(TableName.GetValue()));
		if (TablePtr.IsValid())
		{
			// Find the entry
			const FStringTable* Table = TablePtr.Get();
			FStringTableEntryConstPtr EntryPtr = Table->FindEntry(FTextKey(Key.ToString()));
			if (EntryPtr.IsValid())
			{
				const FStringTableEntry* TableEntry = EntryPtr.Get();
				SourceString = FText::FromString(TableEntry->GetSourceString());
			}
		}

		if (!SourceString.IsEmpty() && !SourceString.EqualTo(MissingEntry) && !SourceString.EqualTo(Key))
		{
			if (ResolveTextExtension)
			{
				return ResolveText(Outer, &SourceString);
			}
			return FText(SourceString);
		}

		// By default, return via the key
		if (ResolveTextExtension && !Key.ToString().EndsWith(".PreviewText"))
		{
			return ResolveText(Outer, &Key);
		}

		// Return backup text, if relevant
		if (BackupText)
		{
			return *BackupText;
		}

		return Key;
	}

protected:
	bool bDataLoaded = false;
	bool bListenerSet = false;
};
