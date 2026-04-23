# Scripting

Articy:draft features its own scripting language, called **articy:expresso**, which is used for controlling narrative logic, conditions, and variables. The **ArticyXImporter** plugin imports these scripts into Unreal Engine, giving developers the flexibility to utilize them in both **Blueprint** and **C++**. Additionally, the **ArticyFlowPlayer** component leverages these scripts when traversing flows, handling conditions and instructions automatically.

This section provides an overview of articy:expresso and how to make use of its scripts within Unreal Engine.

### Scripting with articy:expresso

Articy:expresso distinguishes between two types of scripts:

- **Condition**: A boolean expression that evaluates to `true` or `false`.
- **Instructions**: A set of statements, usually used to change variables or invoke functions.

In **articy:draft**, scripts can be defined in three contexts:

1. **Pins**: Conditions are used in input pins to control the flow, while instructions are defined on output pins.
2. **Condition and Instruction nodes**: Dedicated nodes to define the two types of scripts.
3. **Script template properties**: Properties within a template that define whether a script is a condition, instruction, or has no validation.

### Condition Scripts

A condition script evaluates to either `true` or `false`. An empty condition defaults to `true`. You can use **logical** and **comparison operators** in your conditions. For example:

- `true`
- `GameState.awake`
- `!Inventory.hasKey || Inventory.lockPicks == 0`
- `Resources.woodSupply > 100 && Resources.ironSupply > 50`

Condition scripts can be used in various parts of your game logic to control flow or trigger certain actions.

### Instruction Scripts

Instruction scripts consist of assignment statements, typically used to modify global variables or call functions. For example:

```expresso
GameState.awake = true;
Resources.woodSupply -= 100;
playSound("forge_build.wav");
```

Instructions can alter global variables, trigger functions, and provide additional flexibility within the game logic.

---

### Accessing Objects, Properties, and Templates

Articy:expresso scripts can also access and manipulate object properties and template data. There are built-in functions for querying and modifying properties, incrementing and decrementing values, and even generating random values.

Here are some of the key functions:

#### `getObj(identifier : string, instanceId : int = 0) : ObjectRef`
This function retrieves an object by its technical name or ID:

```expresso
getObj("Chr_Manfred")
getObj("0x01000001000010C6")
```

#### `getProp(obj : ObjectRef, propertyName : string) : Variable`
To access a specific property of an object, use `getProp`:

```expresso
getProp(getObj("Chr_Manfred"), "DisplayName")
```

You can also access template data:

```expresso
getProp(getObj("Chr_Manfred"), "Morale.MoraleValue") > 10
```

#### `setProp(obj : ObjectRef, propertyName : string, value : Variable)`
To change the value of a property:

```expresso
setProp(getObj("Chr_Manfred"), "DisplayName", "Player1")
```

For template properties:

```expresso
setProp(getObj("Chr_Manfred"), "Player_Character.Morale", 100)
```

#### `incrementProp(obj : ObjectRef, propertyName : string, value : Variable = 1)`
Increments a property by the specified value:

```expresso
incrementProp(getObj("Chr_Manfred"), "Player_Character.Morale", 10)
```

#### `random(min : Variable, max : Variable) : Variable`
Generates a random value between the specified range:

```expresso
incrementProp(getObj("Player"), "Inventory.Gold", random(50, 100))
```

---

### Using articy:expresso in Unreal Engine

The **ArticyXImporter** plugin brings the power of articy:expresso into Unreal Engine, allowing you to access and manipulate these scripts in both **Blueprint** and **C++**.

@tabs{
@tab{ C++ | cpp |
```cpp
FArticyId ManfredId = FArticyId("Chr_Manfred");
UArticyObject* Manfred = UArticyDatabase::Get(WorldContext)->GetObject(ManfredId);

// Set a property in C++
UArticyExpressoScripts* Expresso = UArticyDatabase::Get(WorldContext)->GetExpresso();
Expresso->SetProp(Manfred, "Player_Character.Morale", 100);
```

You can also evaluate conditions and execute scripts when traversing the flow using the **ArticyFlowPlayer**:

```cpp
UArticyFlowPlayer* FlowPlayer = NewObject<UArticyFlowPlayer>();
FlowPlayer->SetStartNodeById(ManfredId);
FlowPlayer->PlayBranch(Branch);
```
}
@tab{ Blueprint | uebp |
1. **Get Object and Modify Property:**
   - Use the **Get Object by Name** or **Get Object by ID** nodes in Blueprint to retrieve Articy objects.
   - Then, use the **Set Property** node to modify object properties, such as changing `DisplayName` or modifying template data.

2. **Random Values:**
   - Use the **Random Integer in Range** node in Blueprint for random values in **articy:expresso** conditions and instructions.
}
}

---

**Articy:expresso** scripts offer powerful functionality for controlling your game's logic and narrative, from modifying variables to checking conditions. With the **ArticyXImporter** plugin, these scripts can be used seamlessly within both **C++** and **Blueprint** in Unreal Engine, giving you full control over your game's logic and flow.

For more advanced scripting techniques and working with conditions and instructions in flow traversal, check out the [Dialogues and Flow Traversal](dialogues_and_flow_traversal.md) section.
