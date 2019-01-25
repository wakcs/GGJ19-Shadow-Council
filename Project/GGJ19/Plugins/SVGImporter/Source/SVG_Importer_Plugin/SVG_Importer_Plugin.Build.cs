// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SVG_Importer_Plugin : ModuleRules
{
	public SVG_Importer_Plugin(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage                = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile    = "Public/SVG_Importer_Plugin.h";

        bFasterWithoutUnity     = true;
		
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Private"));

        PublicDependencyModuleNames.AddRange(
            new string[] 
			{
				"Core",
				"SequenceRecorder"
			}
        );

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
                "PropertyEditor",
                "EditorStyle"
			}
		);
	}
}
