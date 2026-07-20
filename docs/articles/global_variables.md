\page globalVariables Global Variables

### Multiple Global Variable Sets

Some games may require having multiple independent sets of global variables, such as each player having their own variable set.

This is supported via the `Override GV` property of the `ArticyFlowPlayer` component.

To create a new, independent set of global variables, right-click in your Content window and find `Alternative Articy Global Variables`.

![](create-alternative-globals.png)

Now, you can simply set the `Override GV` property on your flow player to this asset. Any two flow players with the same setting will share variables, and any flow players with this property unset will share the default global variables.

![](assign-alternative-globals.png)

Similar to the default global variables set, these new sets respect the `Keep global variables between worlds` setting of your project. If it's turned on, changes to these global variables will persist across level boundaries. If it is turned off, each will reset to their default values anytime the level changes.

#### Getting Variable Sets in Blueprint and C++

If you want to access the values in these sets in Blueprint or C++, you need to use the `Get Runtime GVs` method/node on the Articy Database. The `Alternative Articy Global Variables` asset is just a dummy placeholder, so it has no data itself. You need to use this method to access the runtime data.

![](get-vars-bp.png)

Pass the asset reference into the `Get Runtime GVs` method and it will return the active runtime clone for that set.

#### Getting the Current Variables during Custom Script Calls

If you're writing a handler for a [custom script method](#custom-script-methods), you may want to access the variable set currently being used in execution.

When an expresso script is running, the `Get GVs` method/Blueprint node on the Articy Database will return the *active global variables instance* that the flow player is using.

![](get-script-gvs-bp.png)

### Articy Global Variables Debugger
The Global Variables debugger can be accessed in the toolbar at the top of the level editor (UE4) or the Settings menu on the right hand side of the level editor (UE5). It shows all global variables while the game is running and lets you search by namespace or variable name which makes it easy to follow what is happening inside the game and to debug problems in relation to global variables.

Furthermore, you can also change the global variables while the game is running, and your game code that listens to variable changes is going to get triggered. This is useful to replicate specific conditions without needing to go through all steps manually.

For example, if your global variables control your quest states, checking a "quest accepted" global variable in the debugger will make your quest system initiate a quest.
