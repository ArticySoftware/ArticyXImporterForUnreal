//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyLocalizationSubsystem.h"
#include "ArticyLocalizerSystem.h"
#include "ArticyRuntimeModule.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectIterator.h"

UArticyLocalizationSubsystem* UArticyLocalizationSubsystem::Get()
{
    return GEngine ? GEngine->GetEngineSubsystem<UArticyLocalizationSubsystem>() : nullptr;
}

void UArticyLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Defer to OnPostEngineInit: engine subsystems init before the project
    // module, string-table and culture infrastructure is ready, and a broken
    // localization state there crashes the generated Reload().
    if (GIsRunningUnattendedScript || IsRunningCommandlet())
    {
        // Commandlets never reach OnPostEngineInit.
        InitializeLocalizer();
        return;
    }

    PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddUObject(
        this, &UArticyLocalizationSubsystem::InitializeLocalizer);
}

void UArticyLocalizationSubsystem::Deinitialize()
{
    if (PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
        PostEngineInitHandle.Reset();
    }

    Localizer = nullptr;
    Super::Deinitialize();
}

UArticyLocalizerSystem* UArticyLocalizationSubsystem::GetLocalizer() const
{
    return Localizer;
}

void UArticyLocalizationSubsystem::InitializeLocalizer()
{
    if (Localizer)
    {
        return;
    }

    Localizer = CreateGeneratedLocalizer();
    if (!Localizer)
    {
        UE_LOG(LogArticyRuntime, Warning,
            TEXT("ArticyLocalizationSubsystem: no generated UArticyLocalizerSystem subclass found; string table localization disabled."));
        return;
    }

    Localizer->Reload();
}

UArticyLocalizerSystem* UArticyLocalizationSubsystem::CreateGeneratedLocalizer()
{
    UClass* ParentClass = UArticyLocalizerSystem::StaticClass();

    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* Class = *It;

        if (!Class ||
            Class == ParentClass ||
            !Class->IsChildOf(ParentClass) ||
            Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }

        // Skip reinstanced/hot-reloaded/skeleton classes, instantiating them
        // yields partially-initialized objects.
        const FString ClassName = Class->GetName();
        if (ClassName.StartsWith(TEXT("REINST_")) ||
            ClassName.StartsWith(TEXT("TRASHCLASS_")) ||
            ClassName.StartsWith(TEXT("HOTRELOADED_")) ||
            ClassName.StartsWith(TEXT("SKEL_")))
        {
            continue;
        }

        return NewObject<UArticyLocalizerSystem>(this, Class);
    }

    return nullptr;
}
