//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ArticyLocalizationSubsystem.generated.h"

class UArticyLocalizerSystem;

/**
 * Engine subsystem that owns the generated UArticyLocalizerSystem singleton and
 * drives its string-table reload on startup and on culture changes.
 */
UCLASS()
class ARTICYRUNTIME_API UArticyLocalizationSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Retrieves the subsystem instance from GEngine.
	 *
	 * @return The subsystem instance, or nullptr if GEngine is not yet available.
	 */
	static UArticyLocalizationSubsystem* Get();

	/**
	 * Initializes the subsystem. Defers the first localizer Reload() to
	 * OnPostEngineInit so it runs once all modules and string-table infrastructure
	 * are ready; commandlets fall back to synchronous init.
	 *
	 * @param Collection The subsystem collection this subsystem belongs to.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** Tears down the subsystem and unbinds any deferred-init handles. */
	virtual void Deinitialize() override;

	/**
	 * Retrieves the generated UArticyLocalizerSystem instance owned by this subsystem.
	 *
	 * @return The localizer instance, or nullptr if none is available.
	 */
	UArticyLocalizerSystem* GetLocalizer() const;

private:
	/** The generated UArticyLocalizerSystem instance, created on first post-engine-init. */
	UPROPERTY()
	TObjectPtr<UArticyLocalizerSystem> Localizer;

<<<<<<< Updated upstream
=======
	/** Handle to the deferred OnPostEngineInit callback used to run the first Reload(). */
	FDelegateHandle PostEngineInitHandle;

	/** Creates the generated localizer (if not already created) and performs the initial Reload(). */
	void InitializeLocalizer();
	/**
	 * Finds the first valid generated UArticyLocalizerSystem subclass and instantiates it.
	 *
	 * @return A new generated localizer instance, or nullptr if no suitable subclass was found.
	 */
>>>>>>> Stashed changes
	UArticyLocalizerSystem* CreateGeneratedLocalizer();
};
