//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyArchiveReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_AUTOMATION_TESTS

// Verifies the Articy archive reader against a synthetic in-memory archive
BEGIN_DEFINE_SPEC(FArticyArchiveReaderSpec, "Articy.Editor.ArchiveReader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyArchiveReaderSpec)

void FArticyArchiveReaderSpec::Define()
{
	It("opens an Articy archive and reads a stored file back", [this]()
	{
		const FString Content = TEXT("{\"Hello\":\"World\"}");
		const FTCHARToUTF8 ContentUtf8(*Content);
		const int32 ContentLen = ContentUtf8.Length();
		const char* Name = "test.json";
		const int16 NameLen = 9;

		TArray<uint8> Bytes;
		auto Append = [&Bytes](const void* Ptr, int32 Num)
		{
			Bytes.Append(reinterpret_cast<const uint8*>(Ptr), Num);
		};

		// Header (20 bytes): magic, version, pad, flags(u16), numFiles(i32), dictPos(u64).
		Append("ADFA", 4); // "ADFA" file magic
		Bytes.Add(1); // version
		Bytes.Add(0); // pad
		const uint16 Flags = 0;         Append(&Flags, 2);
		const int32 NumFiles = 1;       Append(&NumFiles, 4);
		const uint64 DictPos = 20 + ContentLen; Append(&DictPos, 8);

		// File content at offset 20.
		Append(ContentUtf8.Get(), ContentLen);

		// Dictionary entry: startPos(u64), unpacked(i64), packed(i64), flags(i16), nameLen(i16), name.
		const uint64 StartPos = 20;     Append(&StartPos, 8);
		const int64 Unpacked = ContentLen; Append(&Unpacked, 8);
		const int64 Packed = ContentLen;   Append(&Packed, 8);
		const int16 EntryFlags = 0;     Append(&EntryFlags, 2);
		Append(&NameLen, 2);
		Append(Name, NameLen);

		const FString Path = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("ArticyArchive_"), TEXT(".articyue"));
		FFileHelper::SaveArrayToFile(Bytes, *Path);

		UArticyArchiveReader* Reader = NewObject<UArticyArchiveReader>();
		if (TestTrue(TEXT("archive opened"), Reader->OpenArchive(Path)))
		{
			FString Out;
			TestTrue(TEXT("file read"), Reader->ReadFile(TEXT("test.json"), Out));
			TestEqual(TEXT("content matches"), Out, Content);
		}

		IFileManager::Get().Delete(*Path);
	});
}

#endif // WITH_AUTOMATION_TESTS
