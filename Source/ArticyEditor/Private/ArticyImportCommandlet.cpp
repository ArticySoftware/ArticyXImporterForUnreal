#include "ArticyImportCommandlet.h"
#include "ArticyEditorFunctionLibrary.h"

/**
 * Main function executed by the commandlet.
 *
 * @param Params Command line parameters passed to the commandlet.
 * @return int32 Status code representing the result of the commandlet execution.
 */
int32 UArticyImportCommandlet::Main(const FString& Params)
{
    // Parse the command line parameters into tokens and switches
    TArray<FString> Tokens, Switches;
    ParseCommandLine(*Params, Tokens, Switches);

    // Determine which process to follow based on the switches
    bool CompleteReimport = false;
    bool RegenerateAssets = false;

    // Check each switch to see which operation to perform
    for (int SwitchNum = 0; SwitchNum < Switches.Num(); SwitchNum++)
    {
        if (Switches[SwitchNum].Compare(TEXT("ArticyReimport"), ESearchCase::IgnoreCase) == 0)
        {
            CompleteReimport = true;  // Set flag for complete reimport
        }
        if (Switches[SwitchNum].Compare(TEXT("ArticyRegenerate"), ESearchCase::IgnoreCase) == 0)
        {
            RegenerateAssets = true;  // Set flag for asset regeneration
        }
    }

    GIsRunningUnattendedScript = true;

    int32 Outcome;
    // Execute the appropriate process based on the flags set
    if (CompleteReimport)
    {
        Outcome = FArticyEditorFunctionLibrary::ForceCompleteReimport();  // Perform complete reimport
    }
    else if (RegenerateAssets)
    {
        Outcome = FArticyEditorFunctionLibrary::RegenerateAssets();  // Regenerate assets
    }
    else
    {
        Outcome = FArticyEditorFunctionLibrary::ReimportChanges();  // Default to reimporting changes
    }

    GIsRunningUnattendedScript = false;
    
    return Outcome;
}
