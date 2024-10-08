//
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.
//

#include "BuildToolParser.h"
#include "ArticyEditorModule.h"

/**
 * @brief Constructs a BuildToolParser object and sets the file path.
 * @param filePath The path to the build tool file.
 */
BuildToolParser::BuildToolParser(const FString& filePath)
{
    Path = filePath;
}

/**
 * @brief Verifies if the Articy runtime reference is present in the build tool file.
 *
 * Opens the file specified by the path, removes comments, and checks for
 * the presence of Articy runtime in the dependency module names.
 *
 * @return true if the Articy runtime reference is found, false otherwise.
 */
bool BuildToolParser::VerifyArticyRuntimeRef()
{
    // Open the Path file and read its content as one line string
    FString fileString;
    if (!FFileHelper::LoadFileToString(fileString, *Path))
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to load file '%s' to string"), *Path);
        return false;
    }

    // Extract PublicDependencyModuleNames C# code lines
    FString UncommentedString = RemoveComments(fileString);
    return CheckReferences(UncommentedString);
}

/**
 * @brief Adds the Articy runtime reference to the build tool file if it is not already present.
 *
 * Opens the file, checks for the PublicDependencyModuleNames section, and
 * inserts the Articy runtime reference if it's missing.
 */
void BuildToolParser::AddArticyRuntimmeRef()
{
    // Open the Path file and read its content as one line string
    FString fileString;
    if (!FFileHelper::LoadFileToString(fileString, *Path))
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to load file '%s' to string"), *Path);
        return;
    }

    // Same logic than removing comments => TODO : generalize the whole process...
    FString MatchRule(TEXT("/\\*(.*?)\\*/"));                            // blockComments
    MatchRule.Append(TEXT("|//(.*?)\\r?\\n"));                            // lineComments
    MatchRule.Append(TEXT("|\"\"((\\\\[^\\n]|[^""\\n])*)\"\""));        // strings
    MatchRule.Append(TEXT("|@(\"\"[^\"\"]*\"\")+"));                    // C# verbatimStrings
    MatchRule.Append(TEXT("|PublicDependencyModuleNames[\\s\\S]*?\\{"));    // Dependency array

    const FRegexPattern RegexPattern(*MatchRule);

    FRegexMatcher RegexMatcher(RegexPattern, fileString);

    while (RegexMatcher.FindNext())
    {
        FString TestLine = fileString.Mid(RegexMatcher.GetMatchBeginning(), RegexMatcher.GetMatchEnding() - RegexMatcher.GetMatchBeginning());
        if (TestLine.StartsWith("PublicDependencyModuleNames"))
        {
            fileString.InsertAt(RegexMatcher.GetMatchEnding(), TEXT("\"ArticyRuntime\","));
        }
    }

    FFileHelper::SaveStringToFile(fileString, *Path);
}

/**
 * @brief Removes single and multi-line C# comments from the given string.
 *
 * Utilizes regular expressions to identify and remove comments, leaving other content unchanged.
 *
 * @param line The string from which comments must be removed.
 * @return A new string with comments removed.
 */
FString BuildToolParser::RemoveComments(FString line)
{
    FString StrReturn(line);
    FString CommentMatchRule(TEXT("/\\*(.*?)\\*/"));                    // blockComments
    CommentMatchRule.Append(TEXT("|//(.*?)\\r?\\n"));                    // lineComments
    CommentMatchRule.Append(TEXT("|\"\"((\\\\[^\\n]|[^""\\n])*)\"\"")); // strings
    CommentMatchRule.Append(TEXT("|@(\"\"[^\"\"]*\"\")+"));                // C# verbatimStrings

    const FRegexPattern CsharpComments(*CommentMatchRule);

    FRegexMatcher CommentsRemover(CsharpComments, StrReturn);
    int64 offset = 0;
    while (CommentsRemover.FindNext())
    {
        int32 MatchBegin = CommentsRemover.GetMatchBeginning() - offset;

        if (StrReturn[MatchBegin] == '/')
        {
            if (MatchBegin + 1 <= StrReturn.Len() &&
                (
                    StrReturn[MatchBegin + 1] == '/' ||
                    StrReturn[MatchBegin + 1] == '*'
                    )
                )
            {
                // Regex found a comment => strip the whole match from string
                StrReturn.RemoveAt(MatchBegin, CommentsRemover.GetMatchEnding() - CommentsRemover.GetMatchBeginning());
                offset = offset + (CommentsRemover.GetMatchEnding() - CommentsRemover.GetMatchBeginning());
            }
        }
        // => else, do not touch literal strings (regex matched on " or @)
    }

    return StrReturn;
}

/**
 * @brief Checks if the Articy runtime reference is present in the dependency modules.
 *
 * Searches for the PublicDependencyModuleNames section in the given string
 * and verifies the presence of the Articy runtime reference.
 *
 * @param line The string to search for dependency references.
 * @return true if Articy runtime is found in the dependencies, false otherwise.
 */
bool BuildToolParser::CheckReferences(FString line)
{
    FString PDependency(TEXT("PublicDependencyModuleNames"));    // Method to find
    PDependency.Append(TEXT("[\\s\\S]*?"));                        // Content
    PDependency.Append(TEXT("\\}\\r?\\n*\\)\\r?\\n*;"));        // End delimiter

    const FRegexPattern PdependencyPattern(*PDependency);

    FRegexMatcher DependencyExtractor(PdependencyPattern, line);

    while (DependencyExtractor.FindNext())
    {
        FString TestLine = line.Mid(DependencyExtractor.GetMatchBeginning(), DependencyExtractor.GetMatchEnding() - DependencyExtractor.GetMatchBeginning());
        if (TestLine.Contains("ArticyRuntime"))
        {
            return true;
        }
    }

    return false;
}
