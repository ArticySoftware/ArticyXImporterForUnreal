//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "StringTableGenerator.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "HAL/PlatformFileManager.h"
#include "SourceControlHelpers.h"

static void EscapeCsv(const FString& In, FStringBuilderBase& Out)
{
    for (TCHAR C : In)
    {
        if (C == '"')
        {
            Out.Append(TEXT("\"\""));
        }
        else
        {
            Out.AppendChar(C);
        }
    }
}

void StringTableGenerator::Line(const FString& Key, const FString& SourceString, const FString& PackageId)
{
    TStringBuilder<1024> LineBuilder;

    LineBuilder.AppendChar('"');
    EscapeCsv(Key, LineBuilder);
    LineBuilder.Append(TEXT("\",\""));

    EscapeCsv(SourceString, LineBuilder);
    LineBuilder.Append(TEXT("\",\""));

    EscapeCsv(PackageId, LineBuilder);
    LineBuilder.Append(TEXT("\"\n"));

    FileContent.Append(LineBuilder.ToString());
}

void StringTableGenerator::WriteToFile() const
{
    if (FileContent.IsEmpty())
        return;

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    ISourceControlModule& SCModule = ISourceControlModule::Get();

    bool bCheckOutEnabled = false;
    if (SCModule.IsEnabled())
    {
        bCheckOutEnabled = ISourceControlModule::Get().GetProvider().UsesCheckout();
    }

    // Try to check out the file if it exists
    bool bFileExisted = false;
    if (PlatformFile.FileExists(*Path) && bCheckOutEnabled)
    {
        USourceControlHelpers::CheckOutFile(*Path);
        bFileExisted = true;
    }

    const bool bFileWritten = FFileHelper::SaveStringToFile(FileContent, *Path, FFileHelper::EEncodingOptions::ForceUTF8);

    // Mark the file for addition if it is newly created
    if (!bFileExisted && bFileWritten && SCModule.IsEnabled())
    {
        USourceControlHelpers::MarkFileForAdd(*Path);
    }
}
