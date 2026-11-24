//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyImportStatus.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	/** The facts of a project whose import is complete and current. */
	FArticyImportFacts UpToDateFacts()
	{
		FArticyImportFacts Facts;
		Facts.ArticyDirectory = TEXT("/Game");
		Facts.NumExportFiles = 1;
		Facts.bHasImportData = true;
		Facts.bHasPriorImport = true;
		Facts.bHasGeneratedCode = true;
		Facts.bHasGeneratedAssets = true;
		Facts.ImportedPluginVersion = TEXT("1.6.1");
		Facts.PluginVersion = TEXT("1.6.1");
		return Facts;
	}
}

// Pins which import state the database view reports, and which import it offers for it.
BEGIN_DEFINE_SPEC(FArticyImportStatusSpec, "Articy.Editor.ImportStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyImportStatusSpec)

void FArticyImportStatusSpec::Define()
{
	Describe("Evaluate", [this]()
	{
		It("reports a complete and current import as up to date", [this]()
		{
			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(UpToDateFacts());

			TestTrue(TEXT("up to date"), Status.IsUpToDate());
			TestTrue(TEXT("no message"), Status.GetMessage().IsEmpty());
			TestTrue(TEXT("no action"), Status.GetAction() == EArticyImportAction::None);
		});

		It("reports a missing export when nothing was imported and there is nothing to import", [this]()
		{
			FArticyImportFacts Facts;
			Facts.ArticyDirectory = TEXT("/Game/Articy");

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::NoExportFile);
			TestTrue(TEXT("message names the directory"), Status.GetMessage().ToString().Contains(TEXT("/Game/Articy")));
			TestTrue(TEXT("nothing to import"), Status.GetAction() == EArticyImportAction::None);
		});

		It("offers to import an export that was never imported", [this]()
		{
			FArticyImportFacts Facts;
			Facts.NumExportFiles = 1;

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::NotImported);
			TestTrue(TEXT("action"), Status.GetAction() == EArticyImportAction::ImportChanges);
			TestEqual(TEXT("label"), Status.GetActionLabel().ToString(), FString(TEXT("Import")));
		});

		It("treats import data that was never filled as not imported", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.bHasPriorImport = false;

			TestTrue(TEXT("state"), FArticyImportStatus::Evaluate(Facts).State == EArticyImportState::NotImported);
		});

		It("reports missing generated code as an error fixed by a full reimport", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.bHasGeneratedCode = false;

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::Incomplete);
			TestTrue(TEXT("is an error"), Status.IsError());
			TestTrue(TEXT("message names the code"), Status.GetMessage().ToString().Contains(TEXT("code")));
			TestTrue(TEXT("action"), Status.GetAction() == EArticyImportAction::FullReimport);
		});

		It("reports missing generated assets as an error fixed by a full reimport", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.bHasGeneratedAssets = false;

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::Incomplete);
			TestTrue(TEXT("message names the assets"), Status.GetMessage().ToString().Contains(TEXT("assets")));
			TestTrue(TEXT("action"), Status.GetAction() == EArticyImportAction::FullReimport);
		});

		It("asks for a full reimport after the plugin was updated", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.ImportedPluginVersion = TEXT("1.6.0");

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::PluginChanged);
			TestFalse(TEXT("is an error"), Status.IsError());
			TestTrue(TEXT("message names both versions"), Status.GetMessage().ToString().Contains(TEXT("1.6.0"))
				&& Status.GetMessage().ToString().Contains(TEXT("1.6.1")));
			TestTrue(TEXT("action"), Status.GetAction() == EArticyImportAction::FullReimport);
		});

		It("offers to import the changes of an export that changed", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.bExportChanged = true;

			const FArticyImportStatus Status = FArticyImportStatus::Evaluate(Facts);

			TestTrue(TEXT("state"), Status.State == EArticyImportState::ExportChanged);
			TestTrue(TEXT("action"), Status.GetAction() == EArticyImportAction::ImportChanges);
			TestEqual(TEXT("label"), Status.GetActionLabel().ToString(), FString(TEXT("Import Changes")));
		});

		It("puts a broken import before an outdated one", [this]()
		{
			FArticyImportFacts Facts = UpToDateFacts();
			Facts.bHasGeneratedAssets = false;
			Facts.bExportChanged = true;
			Facts.ImportedPluginVersion = TEXT("1.6.0");

			TestTrue(TEXT("state"), FArticyImportStatus::Evaluate(Facts).State == EArticyImportState::Incomplete);
		});
	});

	Describe("HasLanguage", [this]()
	{
		It("finds the exact culture", [this]()
		{
			FArticyImportFacts Facts;
			Facts.Languages = { TEXT("de"), TEXT("en") };

			TestTrue(TEXT("en"), Facts.HasLanguage(TEXT("en")));
			TestFalse(TEXT("fr"), Facts.HasLanguage(TEXT("fr")));
		});

		It("finds a regional export for its language, but not the other way round", [this]()
		{
			FArticyImportFacts Regional;
			Regional.Languages = { TEXT("pt-BR") };
			FArticyImportFacts General;
			General.Languages = { TEXT("en") };

			TestTrue(TEXT("pt for pt-BR"), Regional.HasLanguage(TEXT("pt")));
			TestFalse(TEXT("en-US for en"), General.HasLanguage(TEXT("en-US")));
		});

		It("assumes every language when none are known", [this]()
		{
			TestTrue(TEXT("any"), FArticyImportFacts().HasLanguage(TEXT("fr")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
