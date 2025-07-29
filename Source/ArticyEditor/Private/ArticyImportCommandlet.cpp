#include "ArticyImportCommandlet.h"
#include "ArticyEditorFunctionLibrary.h"

/**
 * Resolves the Articy content directory to use for import/regeneration.
 *
 * First, checks for a command-line override in the form:
 *    -ArticyDir=/Game/Path/To/ArticyContent
 * If present, returns everything after the ‘=’. Otherwise, it scans
 * the project’s Content/ folder for the first .articyue file it finds,
 * converts that file’s parent directory into a virtual “/Game/…” path,
 * and returns it. If neither approach yields a directory, returns empty.
 *
 * @param Tokens    The parsed command-line tokens (not used here).
 * @param Switches  The parsed command-line switches to inspect for “ArticyDir=” override.
 * @return          A valid Unreal virtual path (e.g. “/Game/Articy/Export”) or an empty string on failure.
 */
FString UArticyImportCommandlet::ResolveArticyDirectory(const TArray<FString>& Tokens, const TArray<FString>& Switches)
{
    for (const auto& S : Switches)
    {
        if (S.StartsWith(TEXT("ArticyDir=")))
        {
            return S.RightChop(FMath::Min(S.Find(TEXT("=")) + 1, S.Len()));
        }
    }

    const FString ContentRoot = FPaths::ProjectContentDir();
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFilesRecursive(FoundFiles, *ContentRoot, TEXT("*.articyue"), /*Files=*/true, /*Dirs=*/false);
    if (FoundFiles.Num() > 0)
    {
        FString Full = FoundFiles[0];
        FString RelPath = Full;
        FPaths::MakePathRelativeTo(RelPath, *ContentRoot);
        FString Dir = FPaths::GetPath(RelPath);
        return TEXT("/Game/") + Dir.Replace(TEXT("\\"), TEXT("/"));
    }

    return {};
}

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

    const FString ArticyDir = ResolveArticyDirectory(Tokens, Switches);
    if (ArticyDir.IsEmpty())
    {
        return -2;
    }

    FArticyEditorFunctionLibrary::SetForcedArticyDirectory(ArticyDir);
    UArticyPluginSettings* Settings = GetMutableDefault<UArticyPluginSettings>();
    Settings->ArticyDirectory = ArticyDir;

    // Determine which process to follow based on the switches
    const bool bDoFull = Switches.Contains(TEXT("ArticyReimport"));
    const bool bDoRegen = Switches.Contains(TEXT("ArticyRegenerate"));

    GIsRunningUnattendedScript = true;

    UArticyImportData* ImportData = nullptr;
    if (FArticyEditorFunctionLibrary::EnsureImportDataAsset(&ImportData) == EImportDataEnsureResult::Failure || !ImportData)
    {
        return -1;
    }

    int32 Outcome = 0;
    if (bDoFull)
        Outcome = FArticyEditorFunctionLibrary::ForceCompleteReimport(ImportData);
    if (bDoRegen)
        Outcome = FArticyEditorFunctionLibrary::RegenerateAssets(ImportData);
    if (!bDoFull && !bDoRegen)
        Outcome = FArticyEditorFunctionLibrary::ReimportChanges(ImportData);

    GIsRunningUnattendedScript = false;
    return Outcome;
}
