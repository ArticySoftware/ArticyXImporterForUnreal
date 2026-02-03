//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ArticyLocalizationSubsystem.generated.h"

class UArticyLocalizerSystem;

UCLASS()
class ARTICYRUNTIME_API UArticyLocalizationSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	static UArticyLocalizationSubsystem* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UArticyLocalizerSystem* GetLocalizer() const;

private:
	UPROPERTY()
	TObjectPtr<UArticyLocalizerSystem> Localizer;

	UArticyLocalizerSystem* CreateGeneratedLocalizer();
};
