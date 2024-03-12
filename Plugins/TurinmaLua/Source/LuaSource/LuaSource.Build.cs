// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class LuaSource : ModuleRules
{
	public LuaSource(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDefinitions.Add("LUA_IN_UE=1");
		PublicDefinitions.Add("KF_IN_UE=1");

		PublicIncludePaths.AddRange(
			new string[] {
				"LuaSource/Lua/src"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);


        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

        
        bEnableUndefinedIdentifierWarnings = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
            if (Target.bBuildEditor)
                PublicDefinitions.Add("LUA_BUILD_AS_DLL");
        }

        if (Target.Platform == UnrealTargetPlatform.Android || Target.Platform == UnrealTargetPlatform.Mac
             || Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicDefinitions.Add("LUA_BUILD_AS_DLL");
            PrivateDefinitions.Add("LUA_USE_DLOPEN");
        }

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Lua", "src"));
    }
}
