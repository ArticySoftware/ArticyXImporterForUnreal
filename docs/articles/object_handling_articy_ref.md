# Object Handling with FArticyRef

In this section, we'll show how you can use **ArticyRef** in Unreal to reference Articy objects dynamically, allowing for flexible and reusable code both in C++ and Blueprints.

### Getting Started with ArticyRef

Instead of hardcoding the technical name of Articy objects in your code, you can use **FArticyRef** to manage object references dynamically. This keeps your code generic and more modular.

In Unreal, **FArticyRef** is a structure used to store references to Articy objects. You can define an **ArticyRef** in your C++ class and expose it to the editor for easy configuration.

**Example: Setting up an ArticyRef in C++**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "ArticyRef.h"
#include "MyArticyActor.generated.h"

UCLASS()
class MYPROJECT_API AMyArticyActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
    FArticyRef MyRef;
};
```

After defining `MyRef` as a public field, you can configure it directly in the Unreal Editor. You can use the **Articy Asset Picker** to select objects from the Articy database and set them as the reference.

### Accessing ArticyRef in Code

Once you’ve set a reference to an object in the editor, you can access and use that reference in your C++ code.

**Example: Accessing an ArticyRef**

```cpp
if (MyRef.GetId().IsNull())
{
    UArticyObject* ArticyObject = MyRef.GetObject(GetWorld());
    if (ArticyObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("Object Technical Name: %s"), *ArticyObject->GetTechnicalName().ToString());
    }
}
```

Here, we use `GetId().IsValid()` to ensure the reference has been set. The `GetObject()` method retrieves the Articy object. The returned object is of type `UArticyObject`, which can then be cast to a more specific type.

**Example: Casting to a specific type**

```cpp
if (MyRef.GetId().IsNull())
{
    UManiacManfredCharacter* Manfred = MyRef.GetObject<UManiacManfredCharacter>(GetWorld());
    if (Manfred)
    {
        UE_LOG(LogTemp, Warning, TEXT("Manfred's Display Name: %s"), *Manfred->GetDisplayName().ToString());
    }
}
```

By specifying the desired type, such as `UManiacManfredCharacter`, when calling `GetObject()`, you can directly cast the object and access its properties.

### Type Constraints for ArticyRef

To avoid runtime errors caused by assigning an incorrect object type, you can set type constraints for an **ArticyRef**. This limits the objects that can be assigned to the reference.

**Example: Limiting to a specific type**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy", meta = (ArticyClassRestriction = "UManiacManfredCharacter"))
FArticyRef PlayerCharacter;
```

This restricts the objects shown in the **Articy Asset Picker** to those of type `UManiacManfredCharacter`, helping to ensure that only valid objects are selected.

### Lists and Arrays of ArticyRefs

You can also use lists or arrays of **ArticyRef** to manage multiple references.

**Example: List of ArticyRefs**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articy")
TArray<FArticyRef> SelectableCharacters;
```

This allows you to reference multiple Articy objects, providing flexibility for complex setups.

### Custom Filters for ArticyRef

In addition to type constraints, you can implement custom filters for **ArticyRef** in Unreal. While this is an advanced feature, it provides even more control over which objects can be selected.
