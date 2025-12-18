//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

using System.Collections.Generic;
using UnrealBuildTool;

public class ArticyRuntime : ModuleRules
{
	public ArticyRuntime(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        OptimizeCode = CodeOptimization.Never;

		PublicIncludePaths.AddRange(
			new List<string>
			{
				// ... add other public include paths required here ...
			}
			);

		PrivateIncludePaths.AddRange(
			new List<string>
			{
				// ... add other private include paths required here ...
			}
			);

		PublicDependencyModuleNames.AddRange(
			new List<string>
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
                "MediaAssets",
				"Json",
				"UMG"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new List<string>
			{
				"Projects",
				"InputCore",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new List<string>
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
