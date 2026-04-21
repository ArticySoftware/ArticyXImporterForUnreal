//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Containers/UnrealString.h"
#include "Misc/Paths.h"

/**
 * @class StringTableGenerator
 * @brief A class to generate and manage string tables in CSV format.
 *
 * This class holds a content string that can be written to a CSV file.
 * It is used for generating string tables for localization purposes, where each
 * entry consists of a key and its corresponding source string.
 */
class StringTableGenerator
{
public:

    /**
     * @brief Executes a lambda safely.
     *
     * This method executes a given lambda function. If the lambda is nullptr,
     * it does nothing.
     *
     * @tparam Lambda The type of the lambda function.
     * @param Lamb The lambda function to execute.
     */
    template<typename Lambda>
    void SafeExecute(Lambda Lamb);

    /**
     * @brief Constructs a StringTableGenerator and executes a content generator lambda.
     *
     * This constructor initializes the file path and prepares the CSV header.
     * It then executes the provided lambda function to generate content and writes it to a file.
     *
     * @tparam Lambda The type of the lambda function.
     * @param TableName The name of the string table.
     * @param Culture The culture/language code for localization.
     * @param ContentGenerator A lambda function to generate the content of the string table.
     */
    template<typename Lambda>
    StringTableGenerator(const FString& TableName, const FString& Culture, Lambda ContentGenerator);

    /**
     * @brief Adds a line to the content string.
     *
     * @param Key The key for the string entry.
     * @param SourceString The source string for the entry.
     */
    void Line(const FString& Key = "", const FString& SourceString = "", const FString& PackageId = "");

    const FString& GetPath() const { return Path; }

private:

    /** The file path where the CSV will be saved. */
    FString Path;

    /** The accumulated content of the CSV file. */
    FString FileContent = "";

    /**
     * @brief Writes the content to a file.
     *
     * This method saves the accumulated file content to the specified file path.
     */
    void WriteToFile() const;
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

template<typename Lambda>
void StringTableGenerator::SafeExecute(Lambda Lamb)
{
    Lamb();
}

template<>
inline void StringTableGenerator::SafeExecute(nullptr_t Lamb)
{
    // Do nothing for nullptr lambdas
}

template <typename Lambda>
StringTableGenerator::StringTableGenerator(const FString& TableName, const FString& Culture, Lambda ContentGenerator)
{
    const FString FilePath = TEXT("ArticyContent/Generated") / TableName;
    if (Culture.IsEmpty())
    {
        Path = FPaths::ProjectContentDir() / FilePath;
    }
    else
    {
        Path = FPaths::ProjectContentDir() / TEXT("L10N") / Culture / FilePath;
    }
    Path += TEXT(".csv");

    Line("Key", "SourceString", "PackageId");
    bool bContentWritten = false;
    if (ensure(!std::is_null_pointer<Lambda>::value))
        bContentWritten = ContentGenerator(this) != 0;

    if (bContentWritten)
        WriteToFile();
}
