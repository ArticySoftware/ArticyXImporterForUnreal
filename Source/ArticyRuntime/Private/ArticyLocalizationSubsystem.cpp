//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyLocalizationSubsystem.h"
#include "ArticyLocalizerSystem.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"

UArticyLocalizationSubsystem* UArticyLocalizationSubsystem::Get()
{
    return GEngine ? GEngine->GetEngineSubsystem<UArticyLocalizationSubsystem>() : nullptr;
}

void UArticyLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Localizer = CreateGeneratedLocalizer();
    if (Localizer)
    {
        Localizer->Reload();
    }
}

void UArticyLocalizationSubsystem::Deinitialize()
{
    Localizer = nullptr;
    Super::Deinitialize();
}

UArticyLocalizerSystem* UArticyLocalizationSubsystem::GetLocalizer() const
{
    return Localizer;
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

        return NewObject<UArticyLocalizerSystem>(this, Class);
    }

    return nullptr;
}
