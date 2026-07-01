//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyImportData.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

// Smoke test over trimmed slices of a real articy:draft export (the "Maniac
// Manfred X" demo). Guards against export-format shape drift that hand-authored
// fixtures can't catch. Fixtures live next to this file under Fixtures/.
namespace
{
	bool LoadFixture(const FString& FileName, TSharedPtr<FJsonObject>& OutRoot)
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ArticyXImporter"));
		if (!Plugin.IsValid())
			return false;

		const FString Path = Plugin->GetBaseDir() / TEXT("Source/ArticyEditor/Private/Tests/Fixtures") / FileName;

		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *Path))
			return false;

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		return FJsonSerializer::Deserialize(Reader, OutRoot) && OutRoot.IsValid();
	}
}

BEGIN_DEFINE_SPEC(FArticyExportSmokeSpec, "Articy.Editor.ExportSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyExportSmokeSpec)

void FArticyExportSmokeSpec::Define()
{
	Describe("manifest (Settings + Project)", [this]()
	{
		It("parses the real settings and project sections", [this]()
		{
			TSharedPtr<FJsonObject> Root;
			if (!TestTrue(TEXT("fixture loaded"), LoadFixture(TEXT("manifest_min.json"), Root)))
				return;

			FAdiSettings Settings;
			Settings.ImportFromJson(Root->GetObjectField(TEXT("Settings")));
			TestEqual(TEXT("text formatter"), Settings.set_TextFormatter, FString(TEXT("Unity")));
			TestEqual(TEXT("export version"), Settings.ExportVersion, FString(TEXT("2.0")));

			FArticyProjectDef Project;
			Project.ImportFromJson(Root->GetObjectField(TEXT("Project")), Settings);
			TestEqual(TEXT("name"), Project.Name, FString(TEXT("Maniac Manfred X")));
			TestEqual(TEXT("technical name"), Project.TechnicalName, FString(TEXT("ManiacManfred")));
		});
	});

	Describe("global variables", [this]()
	{
		It("parses the real GameState namespace", [this]()
		{
			TSharedPtr<FJsonObject> Root;
			if (!TestTrue(TEXT("fixture loaded"), LoadFixture(TEXT("global_variables_min.json"), Root)))
				return;

			const TArray<TSharedPtr<FJsonValue>>* GVArray = nullptr;
			if (!TestTrue(TEXT("has GlobalVariables array"), Root->TryGetArrayField(TEXT("GlobalVariables"), GVArray)))
				return;

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyGVInfo Info;
			Info.ImportFromJson(GVArray, Data);

			TestEqual(TEXT("namespace count"), Info.Namespaces.Num(), 1);
			if (Info.Namespaces.Num() == 1)
			{
				TestEqual(TEXT("namespace"), Info.Namespaces[0].Namespace, FString(TEXT("GameState")));
				TestEqual(TEXT("variable count"), Info.Namespaces[0].Variables.Num(), 2);
				TestEqual(TEXT("first variable"), Info.Namespaces[0].Variables[0].Variable, FString(TEXT("awake")));
			}
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
