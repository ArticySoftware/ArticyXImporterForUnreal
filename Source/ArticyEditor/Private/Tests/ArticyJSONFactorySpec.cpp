//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyJSONFactory.h"
#include "ArticyImportData.h"
#include "ArticyPluginSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"

#if WITH_AUTOMATION_TESTS

// Drives the factory's import entry points with a synthetic export archive. FinalizeImport is
// intercepted through its test hook, so nothing is compiled and no assets are written.
namespace
{
	// Packs the given files into the articy archive format (see ArticyArchiveReaderSpec).
	void WriteArchive(const FString& Path, const TArray<TPair<FString, FString>>& Files)
	{
		TArray<uint8> Bytes;
		auto Append = [&Bytes](const void* Ptr, int32 Num)
		{
			Bytes.Append(reinterpret_cast<const uint8*>(Ptr), Num);
		};

		TArray<TArray<uint8>> Contents;
		for (const TPair<FString, FString>& File : Files)
		{
			const FTCHARToUTF8 Utf8(*File.Value);
			TArray<uint8>& Content = Contents.AddDefaulted_GetRef();
			Content.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		}

		// Header (20 bytes): magic, version, pad, flags(u16), numFiles(i32), dictPos(u64).
		uint64 DictPos = 20;
		for (const TArray<uint8>& Content : Contents)
			DictPos += Content.Num();

		Append("ADFA", 4);
		Bytes.Add(1);
		Bytes.Add(0);
		const uint16 Flags = 0;             Append(&Flags, 2);
		const int32 NumFiles = Files.Num(); Append(&NumFiles, 4);
		Append(&DictPos, 8);

		// File contents back to back, then one dictionary entry per file:
		// startPos(u64), unpacked(i64), packed(i64), flags(i16), nameLen(i16), name.
		for (const TArray<uint8>& Content : Contents)
			Bytes.Append(Content);

		uint64 StartPos = 20;
		for (int32 i = 0; i < Files.Num(); ++i)
		{
			const FTCHARToUTF8 Name(*Files[i].Key);
			const int64 Length = Contents[i].Num();
			const int16 EntryFlags = 0;
			const int16 NameLen = static_cast<int16>(Name.Length());
			Append(&StartPos, 8);
			Append(&Length, 8);
			Append(&Length, 8);
			Append(&EntryFlags, 2);
			Append(&NameLen, 2);
			Append(Name.Get(), NameLen);
			StartPos += Length;
		}

		FFileHelper::SaveArrayToFile(Bytes, *Path);
	}

	// The smallest export FinalizeImport accepts: every section it reads, with no packages or types.
	TArray<TPair<FString, FString>> MakeExportFiles(const FString& TechnicalName)
	{
		const FString Manifest = FString::Printf(TEXT(
			"{"
			"\"Settings\":{\"set_TextFormatter\":\"Unity\",\"set_UseScriptSupport\":\"False\",\"ExportVersion\":\"2.1\","
			"\"set_IncludedNodes\":\"Settings, Project, GlobalVariables, ObjectDefinitions, Packages, ScriptMethods, Languages\"},"
			"\"Project\":{\"Name\":\"%s\",\"DetailName\":\"\",\"Guid\":\"00000000-0000-0000-0000-000000000001\",\"TechnicalName\":\"%s\"},"
			"\"Languages\":[{\"CultureName\":\"en\",\"ArticyLanguageId\":127,\"LanguageName\":\"English\",\"IsVoiceOver\":false}],"
			"\"GlobalVariables\":{\"FileName\":\"global_variables.json\",\"Hash\":\"gv\"},"
			"\"ObjectDefinitions\":{\"Types\":{\"FileName\":\"object_definitions.json\",\"Hash\":\"types\"},"
			"\"Texts\":{\"FileName\":\"object_definitions_localization.json\",\"Hash\":\"texts\"}},"
			"\"Packages\":[],"
			"\"ScriptMethods\":{\"FileName\":\"script_methods.json\",\"Hash\":\"methods\"}"
			"}"), *TechnicalName, *TechnicalName);

		TArray<TPair<FString, FString>> Files;
		Files.Emplace(TEXT("global_variables.json"), TEXT("{\"GlobalVariables\":[]}"));
		Files.Emplace(TEXT("object_definitions.json"), TEXT("{\"ObjectDefinitions\":[]}"));
		Files.Emplace(TEXT("object_definitions_localization.json"), TEXT("{}"));
		Files.Emplace(TEXT("script_methods.json"), TEXT("{\"ScriptMethods\":[]}"));
		Files.Emplace(TEXT("manifest.json"), Manifest);
		return Files;
	}

	// ImportFromJson prunes the project's articy string tables, so stay out of projects that have any.
	bool ProjectHasArticyStringTables()
	{
		TArray<FString> Csvs;
		IFileManager::Get().FindFilesRecursive(Csvs, *(FPaths::ProjectContentDir() / TEXT("ArticyContent/Generated")), TEXT("*.csv"), true, false);
		IFileManager::Get().FindFilesRecursive(Csvs, *(FPaths::ProjectContentDir() / TEXT("L10N")), TEXT("*.csv"), true, false, false);
		return Csvs.Num() > 0;
	}
}

BEGIN_DEFINE_SPEC(FArticyJSONFactorySpec, "Articy.Editor.JSONFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	FString ArchiveDir;
	FString ArchivePath;
	FString SavedArticyDirectory;
	bool bSavedVerifyReference = true;
	bool bSavedVerifyReferenceInstance = true;

	// Recorded by the FinalizeImport hook.
	int32 FinalizeCalls = 0;
	UArticyImportData* FinalizedData = nullptr;
	bool bFinalizeAllowRemoval = false;
	FString FinalizedTechnicalName;

	// Runs the archive through UFactory::ImportObject, which is what the editor's import paths call.
	UArticyImportData* ImportThroughFactory(bool& bOutCanceled);
END_DEFINE_SPEC(FArticyJSONFactorySpec)

UArticyImportData* FArticyJSONFactorySpec::ImportThroughFactory(bool& bOutCanceled)
{
	UArticyJSONFactory* Factory = NewObject<UArticyJSONFactory>();
	const FName AssetName = MakeUniqueObjectName(GetTransientPackage(), UArticyImportData::StaticClass(), TEXT("ArticyJSONFactorySpec"));
	bOutCanceled = false;
	UObject* Result = Factory->ImportObject(UArticyImportData::StaticClass(), GetTransientPackage(), AssetName, RF_Transient, ArchivePath, nullptr, bOutCanceled);
	return Cast<UArticyImportData>(Result);
}

void FArticyJSONFactorySpec::Define()
{
	BeforeEach([this]()
	{
		ArchiveDir = FPaths::ProjectIntermediateDir() / TEXT("ArticyJSONFactorySpec");
		ArchivePath = ArchiveDir / TEXT("FactorySpec.articyue");
		IFileManager::Get().MakeDirectory(*ArchiveDir, true);
		WriteArchive(ArchivePath, MakeExportFiles(TEXT("FactorySpec")));

		// The factory persists a changed articy directory to the project config, so match it up front.
		// The ArticyRuntime build-file check would open a dialog; Get() hands out its own instance, so flip both.
		UArticyPluginSettings* Defaults = GetMutableDefault<UArticyPluginSettings>();
		UArticyPluginSettings* Instance = const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get());
		SavedArticyDirectory = Defaults->ArticyDirectory;
		bSavedVerifyReference = Defaults->bVerifyArticyReferenceBeforeImport;
		bSavedVerifyReferenceInstance = Instance->bVerifyArticyReferenceBeforeImport;
		Defaults->ArticyDirectory = FPaths::GetPath(GetTransientPackage()->GetPathName());
		Defaults->bVerifyArticyReferenceBeforeImport = false;
		Instance->bVerifyArticyReferenceBeforeImport = false;

		FinalizeCalls = 0;
		FinalizedData = nullptr;
		bFinalizeAllowRemoval = false;
		FinalizedTechnicalName.Empty();
		UArticyImportData::Test_FinalizeImportOverride = [this](UArticyImportData* Data, bool bAllowRemoval)
		{
			++FinalizeCalls;
			FinalizedData = Data;
			bFinalizeAllowRemoval = bAllowRemoval;
			FinalizedTechnicalName = Data->GetProject().TechnicalName;
			return true;
		};
	});

	AfterEach([this]()
	{
		UArticyImportData::Test_FinalizeImportOverride = nullptr;
		UArticyPluginSettings* Defaults = GetMutableDefault<UArticyPluginSettings>();
		Defaults->ArticyDirectory = SavedArticyDirectory;
		Defaults->bVerifyArticyReferenceBeforeImport = bSavedVerifyReference;
		const_cast<UArticyPluginSettings*>(UArticyPluginSettings::Get())->bVerifyArticyReferenceBeforeImport = bSavedVerifyReferenceInstance;
		IFileManager::Get().DeleteDirectory(*ArchiveDir, false, true);
	});

	Describe("FactoryCreateFile", [this]()
	{
		It("finalizes the initial import so code and assets get generated", [this]()
		{
			if (ProjectHasArticyStringTables())
			{
				AddWarning(TEXT("Skipped: this project already has imported articy content."));
				return;
			}

			bool bCanceled = false;
			UArticyImportData* Data = ImportThroughFactory(bCanceled);

			TestNotNull(TEXT("import data created"), Data);
			TestFalse(TEXT("not canceled"), bCanceled);
			TestEqual(TEXT("FinalizeImport called once"), FinalizeCalls, 1);
			TestTrue(TEXT("finalized the created asset"), FinalizedData == Data);
			TestTrue(TEXT("single-file import allows removal"), bFinalizeAllowRemoval);
			TestEqual(TEXT("archive was parsed before finalizing"), FinalizedTechnicalName, FString(TEXT("FactorySpec")));
		});

		It("cancels the import when finalizing fails", [this]()
		{
			if (ProjectHasArticyStringTables())
			{
				AddWarning(TEXT("Skipped: this project already has imported articy content."));
				return;
			}

			UArticyImportData::Test_FinalizeImportOverride = [this](UArticyImportData*, bool)
			{
				++FinalizeCalls;
				return false;
			};

			bool bCanceled = false;
			UArticyImportData* Data = ImportThroughFactory(bCanceled);

			TestNull(TEXT("no asset returned"), Data);
			TestTrue(TEXT("canceled"), bCanceled);
			TestEqual(TEXT("FinalizeImport called once"), FinalizeCalls, 1);
		});
	});

	Describe("Reimport", [this]()
	{
		It("finalizes the reimported data", [this]()
		{
			if (ProjectHasArticyStringTables())
			{
				AddWarning(TEXT("Skipped: this project already has imported articy content."));
				return;
			}

			bool bCanceled = false;
			UArticyImportData* Data = ImportThroughFactory(bCanceled);
			if (!TestNotNull(TEXT("import data created"), Data))
				return;

			FinalizeCalls = 0;
			FinalizedData = nullptr;
			bFinalizeAllowRemoval = false;

			UArticyJSONFactory* Factory = NewObject<UArticyJSONFactory>();
			TestTrue(TEXT("reimport succeeded"), Factory->Reimport(Data) == EReimportResult::Succeeded);
			TestEqual(TEXT("FinalizeImport called once"), FinalizeCalls, 1);
			TestTrue(TEXT("finalized the same asset"), FinalizedData == Data);
			TestTrue(TEXT("reimport allows removal"), bFinalizeAllowRemoval);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
