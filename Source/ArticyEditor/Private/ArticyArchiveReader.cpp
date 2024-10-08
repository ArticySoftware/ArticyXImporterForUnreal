//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyArchiveReader.h"
#include "ArticyEditorModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"

/**
 * Opens an archive file for reading.
 *
 * @param InArchiveFileName The name of the archive file to open.
 * @return True if the archive was successfully opened and read; otherwise, false.
 */
bool UArticyArchiveReader::OpenArchive(const FString& InArchiveFileName)
{
	ArchiveFileName = InArchiveFileName;

	if (!ReadHeader())
	{
		UE_LOG(LogArticyEditor, Error, TEXT("Could not load archive %s."), *ArchiveFileName);
		return false;
	}

	if (!ReadFileData())
	{
		UE_LOG(LogArticyEditor, Error, TEXT("Could not retrieve file dictionary from archive %s."), *ArchiveFileName);
		return false;
	}

	return true;
}

/**
 * Reads a file from the archive.
 *
 * @param Filename The name of the file to read from the archive.
 * @param OutResult The string that will receive the file's content.
 * @return True if the file was successfully read; otherwise, false.
 */
bool UArticyArchiveReader::ReadFile(const FString& Filename, FString& OutResult) const
{
	IPlatformFile& PlatformFile = FPlatformFileManager().GetPlatformFile();

	if (IFileHandle* FileHandle = PlatformFile.OpenRead(*ArchiveFileName))
	{
		const FArticyArchiveFileData& FileEntry = FileDictionary[Filename];
		uint8* FileBytes = new uint8[FileEntry.PackedLength];

		// Read file data
		FileHandle->Seek(FileEntry.FileStartPos);
		if (!FileHandle->Read(FileBytes, FileEntry.PackedLength))
		{
			UE_LOG(LogArticyEditor, Error, TEXT("Could not read file %s from archive %s."), *Filename, *ArchiveFileName);
			delete FileHandle;
			return false;
		}

		// TODO: Handle decompression
		OutResult = ArchiveBytesToString(FileBytes, FileEntry.PackedLength);
		delete FileHandle;
	}

	return true;
}

/**
 * Reads the header from the archive file.
 *
 * @return True if the header was successfully read; otherwise, false.
 */
bool UArticyArchiveReader::ReadHeader()
{
	IPlatformFile& PlatformFile = FPlatformFileManager().GetPlatformFile();

	if (IFileHandle* FileHandle = PlatformFile.OpenRead(*ArchiveFileName))
	{
		// Store fields individually due to format tweaks and struct alignment
		uint8 HeaderMagic[4];
		uint8 Version;
		uint8 Pad;
		uint16 Flags;
		int32 NumberOfFiles;
		uint64 FileDictionaryPos;

		// Cast to uint8*
		uint8* FlagsBuffer = reinterpret_cast<uint8*>(&Flags);
		uint8* NumberOfFilesBuffer = reinterpret_cast<uint8*>(&NumberOfFiles);
		uint8* FileDictionaryPosBuffer = reinterpret_cast<uint8*>(&FileDictionaryPos);

		// Read header from the file, stopping if something fails
		if (!FileHandle->Read(HeaderMagic, sizeof(HeaderMagic)))
		{
			delete FileHandle;
			return false;
		}
		const FString MagicString = ArchiveBytesToString(HeaderMagic, 4);
		if (!MagicString.Equals(TEXT("ADFA")))
		{
			UE_LOG(LogArticyEditor, Error, TEXT("Archive file is not valid."));
			delete FileHandle;
			return false;
		}

		if (!FileHandle->Read(&Version, 1))
		{
			delete FileHandle;
			return false;
		}
		if (Version != 1)
		{
			UE_LOG(LogArticyEditor, Error, TEXT("Archive file is incorrect version."));
			delete FileHandle;
			return false;
		}

		if (!FileHandle->Read(&Pad, 1))
		{
			delete FileHandle;
			return false;
		}
		if (!FileHandle->Read(FlagsBuffer, sizeof(Flags)))
		{
			delete FileHandle;
			return false;
		}
		if (!FileHandle->Read(NumberOfFilesBuffer, sizeof(NumberOfFiles)))
		{
			delete FileHandle;
			return false;
		}
		if (!FileHandle->Read(FileDictionaryPosBuffer, sizeof(FileDictionaryPos)))
		{
			delete FileHandle;
			return false;
		}

		// Store in the output struct
		Header.Magic = MagicString;
		Header.Version = Version;
		Header.Pad = Pad;
		Header.Flags = Flags;
		Header.NumberOfFiles = NumberOfFiles;
		Header.FileDictionaryPos = FileDictionaryPos;

		// Close the file again
		delete FileHandle;
		return true;
	}

	return false;
}

/**
 * Reads file data from the archive, populating the file dictionary.
 *
 * @return True if the file data was successfully read; otherwise, false.
 */
bool UArticyArchiveReader::ReadFileData()
{
	FileDictionary.Reset();
	IPlatformFile& PlatformFile = FPlatformFileManager().GetPlatformFile();

	if (IFileHandle* FileHandle = PlatformFile.OpenRead(*ArchiveFileName))
	{
		uint64 FileStartPos;
		int64 UnpackedLength;
		int64 PackedLength;
		int16 Flags;
		int16 LengthOfName;

		// Cast to uint8*
		uint8* FileStartPosBuffer = reinterpret_cast<uint8*>(&FileStartPos);
		uint8* UnpackedLengthBuffer = reinterpret_cast<uint8*>(&UnpackedLength);
		uint8* PackedLengthBuffer = reinterpret_cast<uint8*>(&PackedLength);
		uint8* FlagsBuffer = reinterpret_cast<uint8*>(&Flags);
		uint8* LengthOfNameBuffer = reinterpret_cast<uint8*>(&LengthOfName);

		FileHandle->Seek(Header.FileDictionaryPos);

		for (int32 Counter = 0; Counter < Header.NumberOfFiles; Counter++)
		{
			// Read entry from the file, stopping if something fails
			if (!FileHandle->Read(FileStartPosBuffer, sizeof(FileStartPos)))
			{
				delete FileHandle;
				return false;
			}
			if (!FileHandle->Read(UnpackedLengthBuffer, sizeof(UnpackedLength)))
			{
				delete FileHandle;
				return false;
			}
			if (!FileHandle->Read(PackedLengthBuffer, sizeof(PackedLength)))
			{
				delete FileHandle;
				return false;
			}
			if (!FileHandle->Read(FlagsBuffer, sizeof(Flags)))
			{
				delete FileHandle;
				return false;
			}
			if (!FileHandle->Read(LengthOfNameBuffer, sizeof(LengthOfName)))
			{
				delete FileHandle;
				return false;
			}

			// Get the file name
			uint8* Filename = new uint8[LengthOfName];
			if (!FileHandle->Read(Filename, LengthOfName))
			{
				delete[] Filename;
				delete FileHandle;
				return false;
			}
			const FString& FilenameString = ArchiveBytesToString(Filename, LengthOfName);
			delete[] Filename;

			// Store in the output struct
			FArticyArchiveFileData FileEntry;
			FileEntry.FileStartPos = FileStartPos;
			FileEntry.UnpackedLength = UnpackedLength;
			FileEntry.PackedLength = PackedLength;
			FileEntry.Flags = Flags;
			FileEntry.Filename = FilenameString;

			// Add to output map
			FileDictionary.Add(FilenameString, FileEntry);
		}

		// Close the file again
		delete FileHandle;
		return true;
	}

	return false;
}

/**
 * Converts an array of bytes to a string, assuming UTF-8 encoding.
 *
 * @param In The byte array to convert.
 * @param Count The number of bytes in the array.
 * @return The resulting string.
 */
FString UArticyArchiveReader::ArchiveBytesToString(const uint8* In, int32 Count)
{
	if (In == nullptr || Count <= 0)
	{
		return FString();
	}

	// Convert UTF-8 bytes to TCHARs
	FUTF8ToTCHAR UTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(In), Count);

	// Create FString from TCHARs
	return FString(UTF8ToTCHAR.Length(), UTF8ToTCHAR.Get());
}

/**
 * Fetches a JSON object from the archive, verifying the hash for changes.
 *
 * @param JsonRoot The root JSON object to search within.
 * @param FieldName The field name containing the desired JSON object.
 * @param Hash The hash value to check against.
 * @param OutJsonObject The resulting JSON object, if found and changed.
 * @return True if the JSON object was successfully fetched and changed; otherwise, false.
 */
bool UArticyArchiveReader::FetchJson(
	const TSharedPtr<FJsonObject>& JsonRoot,
	const FString& FieldName,
	FString& Hash,
	TSharedPtr<FJsonObject>& OutJsonObject) const
{
	if (!JsonRoot.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject> FileInfo = JsonRoot->GetObjectField(FieldName);
	const FString& NewHash = FileInfo->GetStringField(TEXT("Hash"));
	if (Hash.Equals(NewHash))
	{
		return false;
	}
	Hash = NewHash;

	const FString& FileName = FileInfo->GetStringField(TEXT("FileName"));

	FString Result;
	if (!ReadFile(FileName, Result))
	{
		UE_LOG(LogArticyEditor, Error, TEXT("Failed to load file '%s' to string"), *FileName);
		return false;
	}

	TSharedPtr<FJsonObject> JsonParsed;
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Result);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		OutJsonObject = JsonParsed;
		return true;
	}

	return false;
}
