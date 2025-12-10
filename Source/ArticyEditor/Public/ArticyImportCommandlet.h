#pragma once

#include "Commandlets/Commandlet.h"
#include "ArticyImportCommandlet.generated.h"

/**
 * UArticyImportCommandlet is a custom commandlet class used for importing Articy data.
 */
UCLASS()
class UArticyImportCommandlet : public UCommandlet
{
    GENERATED_BODY()

    /**
     * Main function that processes command line arguments and performs import operations.
     *
     * @param Params Command line parameters passed to the commandlet.
     * @return int32 Status code representing the result of the commandlet execution.
     */
    virtual int32 Main(const FString& Params) override;

protected:
    /**
     * Resolves the Articy content directory to use for import/regeneration.
     *
     * First, checks for a command-line override in the form:
     *    -ArticyDir=/Game/Path/To/ArticyContent
     * If present, returns everything after the '='. Otherwise, it scans
     * the project's Content/ folder for the first .articyue file it finds,
     * converts that file's parent directory into a virtual '/Game/...' path,
     * and returns it. If neither approach yields a directory, returns empty.
     *
     * @param Tokens   The parsed command-line tokens (unused).
     * @param Switches The parsed command-line switches to inspect for 'ArticyDir=' override.
     * @return         A valid Unreal virtual path (e.g. '/Game/Articy/Export') or an empty string on failure.
     */
    static FString ResolveArticyDirectory(const TArray<FString>& Tokens, const TArray<FString>& Switches);
};
