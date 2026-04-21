//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"

/**
 * @class BuildToolParser
 * @brief A class to parse build tool files and manage Articy runtime references.
 */
class BuildToolParser
{
public:
    /**
     * @brief Constructs a BuildToolParser object with the specified file path.
     * @param filePath The path to the build tool file.
     */
    BuildToolParser(const FString& filePath);

    /**
     * @brief Verifies the presence of Articy runtime references in the build tool file.
     * @return true if the Articy runtime reference is found, false otherwise.
     */
    bool VerifyArticyRuntimeRef();

    /**
     * @brief Adds Articy runtime references to the build tool file if not already present.
     */
    void AddArticyRuntimmeRef();

    /**
     * @brief Default destructor for BuildToolParser.
     */
    ~BuildToolParser() {};

private:
    // Enforce using parameterized constructor
    BuildToolParser() {};

    /**
     * @brief Removes single and multi-line C# comments from a string.
     * @param line The string from which comments must be removed.
     * @return A new string with comments removed.
     */
    FString RemoveComments(FString line);

    /**
     * @brief Checks references and returns a list of all dependency module lines from a string.
     * @param line The string without comments.
     * @return true if dependency modules are found, false otherwise.
     */
    bool CheckReferences(FString line);

    int PublicDependencyIndex; ///< Index of public dependency.
    FString Path = TEXT(""); ///< Path to the build tool file.
    TArray<FString> PublicDependencyModulesNames; ///< Names of public dependency modules.
    TArray<FString> FileCodeLines; ///< Lines of code from the file.
};
