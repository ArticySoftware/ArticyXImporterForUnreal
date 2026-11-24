//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyLocalizerSystem.h"
#include "ArticyPluginSettings.h"
#include "ArticyRuntimeModule.h"
#include "Engine/Engine.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "ArticyLocalizerSystem"

FString UArticyLocalizerSystem::GetCurrentLanguage()
{
	return FInternationalization::Get().GetCurrentCulture()->GetName();
}

void UArticyLocalizerSystem::OnLanguageChanged()
{
	ReportedMissingKeys.Empty();

	if (GetDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch)
	{
		RetainedTexts.Empty();
	}
}

void UArticyLocalizerSystem::ClearLocalizationDatabase()
{
	RetainedTexts.Empty();
	ReportedMissingKeys.Empty();
	Reload();
}

bool UArticyLocalizerSystem::ShouldReportMissingTexts() const
{
	if (!GEngine || !GetDefault<UArticyPluginSettings>()->bErrorOnMissingText)
	{
		return false;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return true;
		}
	}

	return false;
}

void UArticyLocalizerSystem::RetainText(const FText& Key, const FText& Text)
{
	if (!GetDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch)
	{
		RetainedTexts.Add(Key.ToString(), Text);
	}
}

const FText* UArticyLocalizerSystem::FindRetainedText(const FText& Key) const
{
	if (RetainedTexts.Num() == 0 || GetDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch)
	{
		return nullptr;
	}

	return RetainedTexts.Find(Key.ToString());
}

void UArticyLocalizerSystem::ReportMissingText(const FText& Key)
{
	const FString KeyString = Key.ToString();

	// Keys have no whitespace; anything else is a literal text passed through
	if (KeyString.IsEmpty() || KeyString.EndsWith(TEXT(".PreviewText")) || KeyString.FindLastCharByPredicate(FChar::IsWhitespace) != INDEX_NONE)
	{
		return;
	}

	if (ReportedMissingKeys.Contains(KeyString) || !ShouldReportMissingTexts())
	{
		return;
	}
	ReportedMissingKeys.Add(KeyString);

	const FString Language = GetCurrentLanguage();
	UE_LOG(LogArticyRuntime, Error, TEXT("Missing text: no string table entry for '%s' in language '%s'."), *KeyString, *Language);

#if WITH_EDITOR
	FMessageLog("PIE").SuppressLoggingToOutputLog(true).Error(FText::Format(
		LOCTEXT("MissingText", "Articy: no string table entry for '{0}' in language '{1}'."),
		FText::FromString(KeyString), FText::FromString(Language)));
#endif
}

#undef LOCTEXT_NAMESPACE
