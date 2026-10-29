\page cloningObjects Cloning Objects

In Unreal, as in Unity, the data imported from **articy:draft** is generally treated as a single instance. This means that any modifications made to an object are persistent across all references to that object. However, in some situations, you may want to create independent copies of an object so that changes do not affect the original. This process is known as **cloning**, and it allows for multiple instances of an object to be handled separately.

In Unreal, cloning is handled similarly to Unity, with instance IDs used to differentiate between the original object and its clones. Below, we’ll explore how to clone objects in Unreal using both C++ and Blueprints.

---

### Basic Cloning

@tabs{
  @tab{ C++ | cpp |

  To clone an object, you can use the `UArticyDatabase::CloneFrom` method. This method creates a copy of an object with a unique instance ID, allowing you to modify the cloned object independently.

  **Example: Cloning a character object**

  ```cpp
  // Clone the object with instance ID 1
  UManiacManfredCharacter* ClonedManfred = UArticyDatabase::Get(WorldContext)->CloneFrom<UManiacManfredCharacter>("Chr_Manfred", 1);

  // Modify the cloned object
  ClonedManfred->DisplayName = FText::FromString("Cloned Manfred");

  // Retrieve the clone using its instance ID
  UManiacManfredCharacter* RetrievedClone = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredCharacter>("Chr_Manfred", 1);
  UE_LOG(LogTemp, Warning, TEXT("Cloned Manfred's Display Name: %s"), *RetrievedClone->DisplayName.ToString());

  // Retrieve the original (base) object without an instance ID
  UManiacManfredCharacter* BaseManfred = UArticyDatabase::Get(WorldContext)->GetObjectByName<UManiacManfredCharacter>("Chr_Manfred");
  UE_LOG(LogTemp, Warning, TEXT("Base Manfred's Display Name: %s"), *BaseManfred->DisplayName.ToString());
  ```

  In this example:
  - [**`CloneFrom`**](@ref UArticyDatabase::CloneFrom) is used to create a clone of "Chr_Manfred" with instance ID `1`.
  - The cloned object is then modified independently.
  - [**`GetObjectByName`**](@ref UArticyDatabase::GetObjectByName) can retrieve either the cloned object by providing its instance ID, or the base object by omitting the instance ID.

  }

  @tab{ Blueprint | uebp |

  Cloning objects in **Blueprint** follows the same principles as C++, but the process is visually represented in the **Blueprint Editor**.

  1. **Clone an Object**: Use the **Clone From** node to clone an object and specify the instance ID for the clone.
  2. **Modify the Clone**: Once the object is cloned, you can modify its properties just like any other object.
  3. **Retrieve the Clone**: Use the **Get Object** node to retrieve the cloned object by specifying the instance ID.

  ![Clone From and Set Display Name on the clone](bp_db_clone1.png)

  Here’s how it works in Blueprint:

  1. Add the **Clone From** node to clone an object, providing the technical name and instance ID.
  2. Use the **Set Property** nodes to modify the clone's properties.
  3. Use the **Get Object** node to retrieve the base object or clone based on the instance ID.

  ![Get Clone using Get Object by Name](bp_db_clone_get.png)

  }
}

---

### Choosing the Instance ID

There are three ways to decide which instance ID a new clone gets:

- Pass the ID yourself. [`CloneFrom`](@ref UArticyDatabase::CloneFrom) returns `nullptr` if that
  instance already exists, while [`GetOrClone`](@ref UArticyDatabase::GetOrClone) returns the
  existing instance instead of creating one.
- Pass `ArticyCloneId::NextFree` (`-1`), which uses the **lowest** instance ID that is not in use
  yet. It therefore fills the gaps left behind by removed clones, and can hand out the same ID
  twice over the lifetime of a session.
- Call [`CloneWithConsecutiveInstanceIdFrom`](@ref UArticyDatabase::CloneWithConsecutiveInstanceIdFrom),
  which always uses the instance ID **after the highest one in use**. Gaps left by removed clones
  are skipped rather than filled.

@tabs{
@tab{ C++ | cpp |
```cpp
UArticyDatabase* DB = UArticyDatabase::Get(WorldContext);

// Explicit: fails if instance 1 already exists
DB->CloneFromByName(TEXT("Chr_Manfred"), 1);

// Lowest free instance ID, filling gaps
DB->CloneFromByName(TEXT("Chr_Manfred"), ArticyCloneId::NextFree);

// One past the highest instance ID, never reusing one
UArticyObject* Next = DB->CloneWithConsecutiveInstanceIdFromByName(TEXT("Chr_Manfred"));

// Which instance IDs exist right now, original (ArticyCloneId::Original) included
TArray<int32> Ids = DB->GetCloneIdsByName(TEXT("Chr_Manfred"));
```
}
@tab{ Blueprint | uebp |
- **Clone From** / **Clone From By Name** take the instance ID, where `-1` means the lowest free one.
- **Clone With Consecutive Instance Id From** / **... By Name** take no ID and count up from the highest.
- **Get Clone Ids** / **Get Clone Ids By Name** list the instance IDs that exist.
}
}

---

### Removing Clones

Clones live in the database until they are removed, so a system that clones per encounter should
clean up after itself.

@tabs{
@tab{ C++ | cpp |
```cpp
UArticyDatabase* DB = UArticyDatabase::Get(WorldContext);

// A single clone
DB->RemoveCloneByName(TEXT("Chr_Manfred"), 1);

// Every clone of one object
DB->RemoveAllClonesByName(TEXT("Chr_Manfred"));

// Every clone in the database
DB->RemoveAllClonesInDatabase();
```
}
@tab{ Blueprint | uebp |
- **Remove Clone** / **Remove Clone By Name** remove one instance and return whether there was one.
- **Remove All Clones** / **Remove All Clones By Name** remove every instance of one object.
- **Remove All Clones In Database** removes every instance of every loaded object.
}
}

> [!note]
> The original object (`ArticyCloneId::Original`, instance ID `0`) is the data imported from
> *articy:draft* and cannot be removed. Clones also cannot be removed while a flow player is
> exploring branches, because the branches being built point at them; the call is refused with a
> warning in that case.

After removing a clone, retrieving it returns `nullptr`, and its instance ID becomes available to
`ArticyCloneId::NextFree` again.

---

### Cloning with ArticyRef in Unreal

The `FArticyRef` type in Unreal also supports cloning. When dealing with references, you can configure how the reference should behave when cloning objects.

1. **Cloning via ArticyRef**: When setting up a reference using an **ArticyRef** property in a Blueprint or C++, you can specify whether to clone the referenced object or simply reference the base object.
2. **Instance ID in ArticyRef**: For clones, you can specify an instance ID directly in the ArticyRef, which will be used to manage and retrieve the correct instance of the cloned object.

In **Blueprint**, the **ArticyRef** field will automatically provide options for configuring whether to reference the base object or a cloned instance, making it easy to manage cloned references.

---

Cloning in Unreal Engine with **articy:draft** data offers flexibility for managing multiple instances of objects, allowing you to modify clones without affecting the base object. Whether you are working in C++ or Blueprint, the process is straightforward, and the **ArticyDatabase** provides all the necessary tools to clone, retrieve, and manage object instances efficiently.
