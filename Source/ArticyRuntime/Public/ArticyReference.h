//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyRef.h"
#include "Components/ActorComponent.h"
#include "ArticyReference.generated.h"

/**
 * Component to hold a single ArticyRef.
 * While ArticyRef can be used directly in scripts, UArticyReference is most useful in generated code
 * or as a straightforward way for designers to attach Articy objects to actors.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ARTICYRUNTIME_API UArticyReference : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Default constructor. Initializes the component without ticking. */
	UArticyReference();

	/**
	 * The ArticyRef that this component holds.
	 * This reference can be set to an ArticyObject and used to fetch or validate the referenced object.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objects")
	FArticyRef Reference;

	/**
	 * Checks if the ID of the referenced object is valid.
	 *
	 * @return True if the referenced object's ID is valid (not zero), otherwise false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Objects")
	bool IsValid();

	/**
	 * Retrieves the ArticyPrimitive object that is referenced by this component.
	 *
	 * @param WorldContext The context used to obtain the Articy object.
	 * @return A pointer to the ArticyPrimitive object if found, otherwise nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Objects")
	UArticyPrimitive* GetObject(const UObject* WorldContext);

	/**
	 * Sets the reference to an Articy object.
	 *
	 * @param Object The Articy object to reference.
	 */
	UFUNCTION(BlueprintCallable, Category = "Objects")
	void SetReference(UArticyObject* Object);
};
