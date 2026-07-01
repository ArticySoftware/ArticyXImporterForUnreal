//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "BuildToolParser/BuildToolParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// Writes content to a unique temporary .Build.cs file and returns its path.
	FString WriteTempBuildFile(const FString& Content)
	{
		const FString Path = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("ArticyTest_"), TEXT(".Build.cs"));
		FFileHelper::SaveStringToFile(Content, *Path);
		return Path;
	}

	const TCHAR* BuildFileWithoutRef =
		TEXT("public class Foo : ModuleRules {\n")
		TEXT("  public Foo(ReadOnlyTargetRules Target) : base(Target) {\n")
		TEXT("    PublicDependencyModuleNames.AddRange(new string[] { \"Core\", \"CoreUObject\", \"Engine\" });\n")
		TEXT("  }\n")
		TEXT("}\n");

	const TCHAR* BuildFileWithRef =
		TEXT("public class Foo : ModuleRules {\n")
		TEXT("  public Foo(ReadOnlyTargetRules Target) : base(Target) {\n")
		TEXT("    PublicDependencyModuleNames.AddRange(new string[] { \"Core\", \"ArticyRuntime\" });\n")
		TEXT("  }\n")
		TEXT("}\n");

	// ArticyRuntime appears only inside a comment, so it must not count as a reference.
	const TCHAR* BuildFileWithRefInComment =
		TEXT("public class Foo : ModuleRules {\n")
		TEXT("  public Foo(ReadOnlyTargetRules Target) : base(Target) {\n")
		TEXT("    // Remember to add \"ArticyRuntime\" here.\n")
		TEXT("    PublicDependencyModuleNames.AddRange(new string[] { \"Core\" });\n")
		TEXT("  }\n")
		TEXT("}\n");
}

BEGIN_DEFINE_SPEC(FBuildToolParserSpec, "Articy.Editor.BuildToolParser",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FBuildToolParserSpec)

void FBuildToolParserSpec::Define()
{
	Describe("VerifyArticyRuntimeRef", [this]()
	{
		It("returns false when ArticyRuntime is absent", [this]()
		{
			const FString Path = WriteTempBuildFile(BuildFileWithoutRef);
			BuildToolParser Parser(Path);
			TestFalse(TEXT("absent"), Parser.VerifyArticyRuntimeRef());
			IFileManager::Get().Delete(*Path);
		});

		It("returns true when ArticyRuntime is present", [this]()
		{
			const FString Path = WriteTempBuildFile(BuildFileWithRef);
			BuildToolParser Parser(Path);
			TestTrue(TEXT("present"), Parser.VerifyArticyRuntimeRef());
			IFileManager::Get().Delete(*Path);
		});

		It("ignores ArticyRuntime mentioned only in a comment", [this]()
		{
			const FString Path = WriteTempBuildFile(BuildFileWithRefInComment);
			BuildToolParser Parser(Path);
			TestFalse(TEXT("comment only"), Parser.VerifyArticyRuntimeRef());
			IFileManager::Get().Delete(*Path);
		});
	});

	Describe("AddArticyRuntimmeRef", [this]()
	{
		It("adds the reference so a re-verify passes", [this]()
		{
			const FString Path = WriteTempBuildFile(BuildFileWithoutRef);

			BuildToolParser Parser(Path);
			Parser.AddArticyRuntimmeRef();

			BuildToolParser Reparse(Path);
			TestTrue(TEXT("present after add"), Reparse.VerifyArticyRuntimeRef());
			IFileManager::Get().Delete(*Path);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
