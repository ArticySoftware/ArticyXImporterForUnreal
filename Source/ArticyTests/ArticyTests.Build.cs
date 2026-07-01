//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

using System.Collections.Generic;
using UnrealBuildTool;

public class ArticyTests : ModuleRules
{
	public ArticyTests(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(
			new List<string>
			{
				"Core",
				"CoreUObject",
				"Engine",
				// Modules under test
				"ArticyRuntime",
				"ArticyEditor"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new List<string>
			{
				"Json",
				"UnrealEd"
			}
			);
	}
}
