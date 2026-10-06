//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyType.h"
#include "ArticyTypeSystem.generated.h"

UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyTypeSystem : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Retrieves the type system instance. This is the generated <Project>TypeSystem asset
	 * if the project has been imported, otherwise an empty placeholder.
	 */
	static UArticyTypeSystem* Get();

	/**
	 * Retrieves the metadata for an articy type by its technical name.
	 *
	 * @param TypeName The technical name of the type, e.g. "Entity" or a template name.
	 * @return The type metadata, or a default-constructed one if the type is unknown.
	 */
	UFUNCTION(BlueprintCallable, Category = "Articy")
	FArticyType GetArticyType(const FString& TypeName) const;

	/** Type metadata by technical name, filled by the importer on the generated asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	TMap<FString, FArticyType> Types;
};
