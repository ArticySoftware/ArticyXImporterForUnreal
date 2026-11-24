//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyType.h"
#include "ArticyTextExtension.h"
#include "Internationalization/StringTableRegistry.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "Internationalization/StringTableCore.h"
#include "ArticyLocalizationSubsystem.h"
#include "ArticyLocalizerSystem.generated.h"

UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyLocalizerSystem : public UObject
{
	GENERATED_BODY()

public:
	static UArticyLocalizerSystem* Get()
	{
		if (UArticyLocalizationSubsystem* Subsys = UArticyLocalizationSubsystem::Get())
		{
			return Subsys->GetLocalizer();
		}
		return nullptr;
	}

	virtual void Reload() {};

	inline FText ResolveText(UObject* Outer, const FText* SourceText)
	{
		return UArticyTextExtension::Get()->Resolve(Outer, SourceText);
	}

	inline FText LocalizeString(UObject* Outer, const FText& Key, bool ResolveTextExtension = true, const FText* BackupText = nullptr, bool bReportMissing = false)
	{
		if (!bDataLoaded)
		{
			// Either return BackupText or Key.
			if (BackupText) return *BackupText;
			return Key;
		}

		const FText MissingEntry = FText::FromString("<MISSING STRING TABLE ENTRY>");

		// Default to key
		FText SourceString = Key;

		// Look up entry in specified string table
		const FString TableName = TEXT("ARTICY");

		// Find the table
		bool bHasEntry = false;
		FStringTableConstPtr TablePtr = FStringTableRegistry::Get().FindStringTable(FName(TableName));
		if (TablePtr.IsValid())
		{
			// Find the entry
			const FStringTable* Table = TablePtr.Get();
			FStringTableEntryConstPtr EntryPtr = Table->FindEntry(FTextKey(Key.ToString()));
			if (EntryPtr.IsValid())
			{
				const FStringTableEntry* TableEntry = EntryPtr.Get();
				SourceString = FText::FromString(TableEntry->GetSourceString());
				bHasEntry = true;
			}
		}

		if (!SourceString.IsEmpty() && !SourceString.EqualTo(MissingEntry) && !SourceString.EqualTo(Key))
		{
			RetainText(Key, SourceString);

			if (ResolveTextExtension)
			{
				return ResolveText(Outer, &SourceString);
			}
			return FText(SourceString);
		}

		if (!bHasEntry)
		{
			// A text of the previous language stands in, if those are kept
			if (const FText* Retained = FindRetainedText(Key))
			{
				return ResolveTextExtension ? ResolveText(Outer, Retained) : *Retained;
			}

			// Only where a text is expected; other callers just try whether a name is a key
			if (bReportMissing)
			{
				ReportMissingText(Key);
			}
		}

		// The key has no usable table entry, so fall back to the backup text if the caller
		// gave one. Resolving the key instead would hide it behind the raw key string.
		const FText& Fallback = BackupText ? *BackupText : Key;

		if (ResolveTextExtension && !Key.ToString().EndsWith(".PreviewText"))
		{
			return ResolveText(Outer, &Fallback);
		}

		return Fallback;
	}

	/** @brief Gets the name of the culture the texts are loaded for, e.g. "en". */
	static FString GetCurrentLanguage();

	/** @brief Called when the language changed. */
	void OnLanguageChanged();

	/** @brief Reloads the string tables and forgets the retained texts. */
	void ClearLocalizationDatabase();

	/** @brief Gets the number of texts kept from before a language switch. */
	int32 GetNumRetainedTexts() const { return RetainedTexts.Num(); }

protected:
	/** @brief Returns true if missing texts are reported: the setting is on and a PIE session is running. */
	virtual bool ShouldReportMissingTexts() const;

	bool bDataLoaded = false;
	bool bListenerSet = false;

private:
	/** Remembers a text, unless texts are cleared on a language switch. */
	void RetainText(const FText& Key, const FText& Text);
	/** Returns the remembered text of a key, or nullptr. */
	const FText* FindRetainedText(const FText& Key) const;
	/** Reports a key without a string table entry, once per key and language. */
	void ReportMissingText(const FText& Key);

	TMap<FString, FText> RetainedTexts;
	TSet<FString> ReportedMissingKeys;
};
