\page projectHierarchy Project Hierarchy

Besides the exported objects, an **articy:draft X** export contains the complete tree of your articy project: every folder, flow node, entity, location, asset, template and settings object, including objects that were **not exported** or whose package is **not loaded**. The **ArticyXImporter** imports this tree and stores it with the [**ArticyDatabase**](@ref UArticyDatabase), so you can rebuild the exact project structure you see in articy, for example to group objects by folder in your own UI.

The hierarchy is exposed through two classes:

- [**UArticyHierarchyManager**](@ref UArticyHierarchyManager) holds the whole tree. Get it from the database with [`GetProjectHierarchy`](@ref UArticyDatabase::GetProjectHierarchy).
- [**UArticyHierarchyNode**](@ref UArticyHierarchyNode) is a lightweight entry for one articy object: its [Id](@ref UArticyHierarchyNode::GetId), the [Id of its parent](@ref UArticyHierarchyNode::GetParent), its [technical name](@ref UArticyHierarchyNode::GetTechnicalName), its [articy type](@ref UArticyHierarchyNode::GetArticyType) (e.g. `FlowFragment`, `Entity`, `UserFolder`) and its [children](@ref UArticyHierarchyNode::GetChildren).

> [!note]
> The hierarchy is only exported when the **Hierarchy** node is included in the export rule set. Projects imported with an importer version without hierarchy support need to be reimported (or have their assets regenerated) before the hierarchy is available.

### Walking the tree

The [project node](@ref UArticyHierarchyManager::GetProjectNode) is the root of the tree and represents the articy project itself. Its parent Id is null.

@tabs{

   @tab{ C++ | cpp |

   ```cpp
   #include "ArticyDatabase.h"
   #include "ArticyHierarchyManager.h"

   void PrintNode(const UArticyHierarchyNode* Node, int32 Depth)
   {
       UE_LOG(LogTemp, Log, TEXT("%s%s (%s)"), *FString::ChrN(Depth * 2, ' '), *Node->GetTechnicalName(), *Node->GetArticyType());
       for (const UArticyHierarchyNode* Child : Node->GetChildren())
       {
           PrintNode(Child, Depth + 1);
       }
   }

   UArticyHierarchyManager* Hierarchy = UArticyDatabase::Get(WorldContext)->GetProjectHierarchy();
   if (UArticyHierarchyNode* ProjectNode = Hierarchy->GetProjectNode())
   {
       PrintNode(ProjectNode, 0);
   }
   ```

   }

   @tab{ Blueprint | uebp |

   - Use **Get Articy DB**, then **Get Project Hierarchy** to get the hierarchy manager.
   - **Get Project Node** returns the root node. Use **Get Children** on a node to iterate over its children, and **Get Technical Name** / **Get Articy Type** to read its data.

   }
}

---

### Looking up a node

[`GetHierarchyInfo`](@ref UArticyHierarchyManager::GetHierarchyInfo) returns the node for an object Id, or null if the Id is not part of the hierarchy. Nodes store the Id of their parent rather than the parent node itself, so walking up the tree is done by looking up the parent Id:

@tabs{

   @tab{ C++ | cpp |

   ```cpp
   UArticyDatabase* Database = UArticyDatabase::Get(WorldContext);
   UArticyHierarchyManager* Hierarchy = Database->GetProjectHierarchy();

   // Build the folder path of an object, e.g. "ManiacManfred/Flow/FFr_Lobby"
   TArray<FString> Path;
   for (UArticyHierarchyNode* Node = Hierarchy->GetHierarchyInfo(Object->GetId()); Node; )
   {
       Path.Insert(Node->GetTechnicalName(), 0);
       Node = Node->GetParent().IsNull() ? nullptr : Hierarchy->GetHierarchyInfo(Node->GetParent());
   }
   const FString FolderPath = FString::Join(Path, TEXT("/"));
   ```

   }

   @tab{ Blueprint | uebp |

   - Call **Get Hierarchy Info** on the hierarchy manager with the Id of an object to get its node.
   - Call **Get Parent** on a node to get the Id of its parent, and pass that Id to **Get Hierarchy Info** again to move up the tree. The project node's parent Id is null.

   }
}

---

### Getting the object of a node

[`GetObject`](@ref UArticyHierarchyNode::GetObject) returns the articy object a node describes, resolved through the database the hierarchy belongs to. It returns null if the object was not exported, if its package is not currently loaded, or, when a class is given, if the object is not of that class. Folders and other nodes without an exported object always return null.

@tabs{

   @tab{ C++ | cpp |

   ```cpp
   for (const UArticyHierarchyNode* Child : Hierarchy->GetProjectNode()->GetChildren())
   {
       if (UArticyObject* Object = Child->GetObject())
       {
           // The object is exported and loaded
       }
   }

   // Typed access, returns null if the object is not a UArticyFlowFragment
   UArticyFlowFragment* Fragment = Node->GetObject<UArticyFlowFragment>();
   ```

   }

   @tab{ Blueprint | uebp |

   - Call **Get Object** on a node. Select a class in **Cast To** to get the object as that type; the result is empty if the object is of a different class.

   }
}
