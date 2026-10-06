//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Engine/Blueprint.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/CoreRedirects.h"

#if WITH_AUTOMATION_TESTS

// The plugin's Blueprints still reference each other through the pre-rename /ArticyImporter/
// paths, so they only load through the PackageRedirects in Config/DefaultArticyXImporter.ini.
// Without them a cook fails with "Create Widget must have a class specified" and friends.
BEGIN_DEFINE_SPEC(FArticyPluginContentSpec, "Articy.Editor.PluginContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPluginContentSpec)

void FArticyPluginContentSpec::Define()
{
	Describe("CoreRedirects from the plugin config", [this]()
	{
		It("redirect the pre-rename content packages to /ArticyXImporter", [this]()
		{
			const TCHAR* Packages[] = {
				TEXT("BP_ArticyFlowDebugger"),
				TEXT("UI/W_ArticyDebugger"),
				TEXT("UI/W_ArticyBranchButton"),
				TEXT("Res/ArticyImporter64"),
				TEXT("Res/ArticyImporter16") };
			for (const TCHAR* Package : Packages)
			{
				const FCoreRedirectObjectName Old(FString::Printf(TEXT("/ArticyImporter/%s"), Package));
				const FCoreRedirectObjectName New = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Package, Old);
				TestEqual(Package, New.PackageName.ToString(), FString::Printf(TEXT("/ArticyXImporter/%s"), Package));
			}
		});

		It("redirect the renamed flow player property", [this]()
		{
			const FCoreRedirectObjectName Old(TEXT("/Script/ArticyRuntime.ArticyFlowPlayer.ExploreDepthLimit"));
			const FCoreRedirectObjectName New = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Property, Old);
			TestEqual(TEXT("ExploreDepthLimit"), New.ObjectName.ToString(), FString(TEXT("ExploreLimit")));
		});
	});

	Describe("flow debugger blueprints", [this]()
	{
		It("load and compile without errors", [this]()
		{
			const TCHAR* Paths[] = {
				TEXT("/ArticyXImporter/BP_ArticyFlowDebugger.BP_ArticyFlowDebugger"),
				TEXT("/ArticyXImporter/UI/W_ArticyDebugger.W_ArticyDebugger"),
				TEXT("/ArticyXImporter/UI/W_ArticyBranchButton.W_ArticyBranchButton") };
			for (const TCHAR* Path : Paths)
			{
				UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
				if (!TestNotNull(Path, Blueprint))
					continue;

				FCompilerResultsLog Results;
				FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);
				TestEqual(Path, Results.NumErrors, 0);
			}
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
