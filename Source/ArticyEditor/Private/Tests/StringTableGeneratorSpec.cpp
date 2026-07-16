//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "StringTableGenerator.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FStringTableGeneratorSpec, "Articy.Editor.StringTableGenerator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FStringTableGeneratorSpec)

void FStringTableGeneratorSpec::Define()
{
	Describe("Path", [this]()
	{
		It("uses ArticyContent/Generated for the default culture", [this]()
		{
			// Returning 0 from the generator means nothing is written to disk.
			StringTableGenerator Gen(TEXT("MyTable"), TEXT(""), [](StringTableGenerator*) { return 0; });
			TestTrue(TEXT("suffix"), Gen.GetPath().EndsWith(TEXT("ArticyContent/Generated/MyTable.csv")));
		});

		It("nests under L10N for a specific culture", [this]()
		{
			StringTableGenerator Gen(TEXT("MyTable"), TEXT("de"), [](StringTableGenerator*) { return 0; });
			TestTrue(TEXT("l10n path"), Gen.GetPath().Contains(TEXT("/L10N/de/")));
		});
	});

	Describe("CSV output", [this]()
	{
		It("writes a header row and CSV-escaped entries", [this]()
		{
			FString Path;
			{
				StringTableGenerator Gen(TEXT("ArticyUnitTestTable"), TEXT(""), [](StringTableGenerator* G)
				{
					G->Line(TEXT("Greeting"), TEXT("Hello"), TEXT("Pkg1"));
					G->Line(TEXT("Quote"), TEXT("a\"b"), TEXT("Pkg1")); // embedded quote -> doubled
					return 1;
				});
				Path = Gen.GetPath();
			}

			FString Content;
			const bool bLoaded = FFileHelper::LoadFileToString(Content, *Path);

			TestTrue(TEXT("file written"), bLoaded);
			TestTrue(TEXT("header"), Content.Contains(TEXT("\"Key\",\"SourceString\",\"PackageId\"")));
			TestTrue(TEXT("row"), Content.Contains(TEXT("\"Greeting\",\"Hello\",\"Pkg1\"")));
			TestTrue(TEXT("escaped quote"), Content.Contains(TEXT("\"a\"\"b\"")));

			IFileManager::Get().Delete(*Path);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
