# Basic Object Handling

When you export your data from **articy:draft X** into your Unreal project, the **ArticyXImporter** plugin populates a database with your data. This section will cover how to access and interact with that data in Unreal Engine using both **C++** and **Blueprint**.

### Simple Object Access

The most basic way to access an Articy object is by querying it directly from the **ArticyDatabase** using its **Id** or **Technical Name**.

**In C++**

**Example: Retrieving an object by its technical name:**

```cpp
UArticyObject* Manfred = UArticyDatabase::Get(WorldContext)->GetObjectByName("Chr_Manfred");
```

[...]
