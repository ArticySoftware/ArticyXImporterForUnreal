//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

using System.Collections.Generic;
using UnrealBuildTool;

/**
 * @class ArticyEditor
 * @brief Defines the build rules for the ArticyEditor module.
 *
 * This class specifies the configuration and dependencies for the ArticyEditor module,
 * which extends Unreal Engine's editor functionality to support Articy assets.
 */
public class ArticyEditor : ModuleRules
{
    /**
     * @brief Constructor for the ArticyEditor module.
     *
     * Initializes the module with the specified target rules, setting up include paths,
     * dependency modules, and code optimization settings.
     *
     * @param Target The target rules for the module.
     */
    public ArticyEditor(ReadOnlyTargetRules target) : base(target)
    {
        // Set precompiled header usage to explicit or shared
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Disable code optimization
        OptimizeCode = CodeOptimization.Never;

        // Add public include paths required for this module
        PublicIncludePaths.AddRange(
            new List<string>
            {
                // ... add public include paths required here ...
            }
            );

        // Add private include paths required for this module
        PrivateIncludePaths.AddRange(
            new List<string>
            {
                // ... add other private include paths required here ...
            }
            );

        // Add public dependencies that are statically linked with this module
        PublicDependencyModuleNames.AddRange(
            new List<string>
            {
                "Core",
                "ArticyRuntime",
                "EditorWidgets",
                "Json",
                "JsonUtilities"
                // "ClassViewer"
                // ... add other public dependencies that you statically link with here ...
            }
            );

        // Add private dependencies that are statically linked with this module
        PrivateDependencyModuleNames.AddRange(
            new List<string>
            {
                "Projects",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                // ... add private dependencies that you statically link with here ...    
                "Json",
                "GameProjectGeneration",
                "ContentBrowser",
                "PropertyEditor",
                "EditorStyle",
                "SourceControl",
                "GraphEditor",
                "AudioEditor",
                "ApplicationCore",
#if UE_5_0_OR_LATER
                "ToolMenus",
#endif
            }
            );

        // Add any modules that this module loads dynamically
        DynamicallyLoadedModuleNames.AddRange(
            new List<string>
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
