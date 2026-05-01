//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "ArticyArchiveReader.generated.h"

/**
 * Structure that represents the header of an Articy archive.
 */
USTRUCT()
struct ARTICYEDITOR_API FArticyArchiveHeader
{
	GENERATED_BODY()

	/** The magic number used to identify the archive file format. */
	FString Magic;
	/** The version of the archive file format. */
	uint8 Version;
	/** Padding byte for alignment. */
	uint8 Pad;
	/** Flags associated with the archive. */
	uint16 Flags;
	/** The number of files contained in the archive. */
	int32 NumberOfFiles;
	/** The position of the file dictionary in the archive. */
	uint64 FileDictionaryPos;
};

/**
 * Structure that represents metadata for a file within an Articy archive.
 */
USTRUCT()
struct ARTICYEDITOR_API FArticyArchiveFileData
{
	GENERATED_BODY()

	/** The starting position of the file within the archive. */
	uint64 FileStartPos;
	/** The unpacked length of the file data. */
	int64 UnpackedLength;
	/** The packed length of the file data. */
	int64 PackedLength;
	/** Flags associated with the file. */
	int16 Flags;
	/** The filename of the file. */
	FString Filename;
};

/**
 * Class responsible for reading and processing Articy archives.
 */
UCLASS()
class ARTICYEDITOR_API UArticyArchiveReader : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Opens an archive file for reading.
	 *
	 * @param InArchiveFileName The name of the archive file to open.
	 * @return True if the archive was successfully opened and read; otherwise, false.
	 */
	bool OpenArchive(const FString& InArchiveFileName);

	/**
	 * Reads a file from the archive.
	 *
	 * @param Filename The name of the file to read from the archive.
	 * @param OutResult The string that will receive the file's content.
	 * @return True if the file was successfully read; otherwise, false.
	 */
	bool ReadFile(const FString& Filename, FString& OutResult) const;

	/**
	 * Converts an array of bytes to a string, assuming UTF-8 encoding.
	 *
	 * @param In The byte array to convert.
	 * @param Count The number of bytes in the array.
	 * @return The resulting string.
	 */
	static FString ArchiveBytesToString(const uint8* In, int32 Count);

	/**
	 * Fetches a JSON object from the archive, verifying the hash for changes.
	 *
	 * @param JsonRoot The root JSON object to search within.
	 * @param FieldName The field name containing the desired JSON object.
	 * @param Hash The hash value to check against.
	 * @param OutJsonObject The resulting JSON object, if found and changed.
	 * @return True if the JSON object was successfully fetched and changed; otherwise, false.
	 */
	bool FetchJson(
		const TSharedPtr<FJsonObject>& JsonRoot,
		const FString& FieldName,
		FString& Hash,
		TSharedPtr<FJsonObject>& OutJsonObject) const;

protected:
	/**
	 * Reads the header from the archive file.
	 *
	 * @return True if the header was successfully read; otherwise, false.
	 */
	bool ReadHeader();

	/**
	 * Reads file data from the archive, populating the file dictionary.
	 *
	 * @return True if the file data was successfully read; otherwise, false.
	 */
	bool ReadFileData();

	/** The name of the archive file. */
	FString ArchiveFileName;
	/** The header information of the archive. */
	FArticyArchiveHeader Header;
	/** The dictionary of files contained in the archive. */
	TMap<FString, FArticyArchiveFileData> FileDictionary;
};
