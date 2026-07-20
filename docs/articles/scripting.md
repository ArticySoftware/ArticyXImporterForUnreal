\page scripting Scripting

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

### Custom Script Methods

It's possible to add new script methods into articy:draft's Expresso scripting language. These can trigger side effects in your game such as moving game objects or changing UI states, or they could return helpful values such as the location of the player.

Getting started with custom script methods is easy.
Simply start using new methods in articy:draft X as if they already exist (as pictured below) and import your project into Unreal.

Please be careful, however, to use only one custom script method per instruction node if you need to guarantee a sequential execution in Unreal for theses instructions.

![](custom-method-node.png)

The importer will detect these methods, infer their parameter types and return signatures (in the example above, one integer and one string), and generate an Interface you can implement in Blueprint or C++ with their implementations.

There are three ways you can implement this interface.

1. Implement the interface on an actor containing an `ArticyFlowPlayer` component. Any time that flow player component encounters a custom method, it'll call the function on this parent actor.
2. Implement the interface on a component deriving from a `ActicyFlowPlayer` component. Any time this flow player encounters a custom method, it'll call the function on itself.
3. Implementing the interface on a custom `UObject` and setting that class as the `User Methods Provider` on a `ArticyFlowPlayer` component. Any time that flow player encounters a custom method, it'll call the function on a new instance of that object.

Each flow player can only use one of the above methods (you can't mix and match).

Choose one and create or open the corresponding Blueprint (whether it's the actor, the component, or the custom `UObject`). Go to `Class Settings` and add the interface generated from your Articy project to the Interfaces list.

![](interface-class-settings.png)

Now, you can start implementing your custom methods. To do this, find the method in the `Interfaces` list under `My Blueprint` (bottom left), right-click it, and select `Implement event`.

![](implement-custom-function.png)

This will create a new event node in your Blueprint graph with all the appropriate parameters.

![](custom-function-node.png)

You'll notice the types of each parameter are automatically deduced based on how you used the function in Articy. Now, attach some nodes (if you just want to test it, try a Debug Print to start) and test it out.

> [!NOTE]
> You may notice your method is called earlier and more often than expected. This is because Articy "scans ahead" in branches to find which ones are valid. To avoid executing your logic twice, see [Shadowing](#Shadowing).

#### Custom Methods that Return

You can also define custom Expresso script methods that have return values.

![](articy-return-custom-method.png)

To create implementations for these in Blueprint, use the Override function method in the Blueprint editor on the object that implements your interface.

![](implement-custom-return-function.png)

Then, you'll get a custom function in Blueprint that can return a value.

![](custom-function-return-blueprint.png)


#### Shadowing

You'll notice that if you put custom script methods into Instructions, Conditions or Pins, the methods will be executed *before* the node is actually reached by the Flow Player.

This is because the flow player scans ahead while figuring out which branches are valid and not. This is how it knows which choices to show and which to hide. In order to make these decisions, it needs to run instructions and conditions ahead of time to see if any fail.

Obviously, this would create problems if any of these scripts modify variables or properties, so the flow player goes into a **shadow state** when doing this. While shadowing, the flow player duplicates the global variables and other state so that changes made by instructions do not affect the real, current state.

All this is handled automatically and requires no input from you.

However, Articy doesn't know what your custom script methods do. If they have side effects (such as changing the state of your game or displaying something on the UI), it doesn't know that these shouldn't be executed during the shadow state.

You need to handle this yourself.

Thankfully, this is easy to handle with the `Is in shadow state?` Blueprint node available on the Articy Database. Gate any side effects your function has behind this method returning `False` to ensure they're only run when the node is actually being executed.

![](check-shadow-state.png)

If your custom function has a return value, however, you still want to make sure it runs as normally. Remember: shadowing is how articy decides what branches are valid or not. If you return a different value while shadowing than you would otherwise, articy won't be able to figure out the proper list of branches to return. Only use `Is in shadow state?` to gate side-effects.

---

**Articy:expresso** scripts offer powerful functionality for controlling your game's logic and narrative, from modifying variables to checking conditions. With the **ArticyXImporter** plugin, these scripts can be used seamlessly within both **C++** and **Blueprint** in Unreal Engine, giving you full control over your game's logic and flow.

For more advanced scripting techniques and working with conditions and instructions in flow traversal, check out the [Dialogues and Flow Traversal](dialogues_and_flow_traversal.md) section.
