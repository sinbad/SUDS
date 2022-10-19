// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SUDSEditor : ModuleRules
{
	public SUDSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
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

		AddAntlr();
		
		// Required for Antlr
		bUseRTTI = true;
		// Antlr Cpp gen doesn't respect UE PCH rules
		PrivatePCHHeaderFile = "Private/SUDSEditorPCH.h";
		PCHUsage = PCHUsageMode.NoSharedPCHs;

	}
	
	private string AntlrPath
	{
		get { return Path.GetFullPath( Path.Combine( ModuleDirectory, "../../antlr/" ) ); }
	}
	

	private bool AddAntlr() 
	{
		bool isLibrarySupported = false;
		bool bDebug = Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame;
		bool bDevelopment = Target.Configuration == UnrealTargetConfiguration.Development;

		if ((Target.Platform == UnrealTargetPlatform.Win64))
		{
			isLibrarySupported = true;

			string BuildFolder = bDebug ? "Debug":
				bDevelopment ? "RelWithDebInfo" : "Release";

			string LibrariesPath = Path.Combine(AntlrPath, "lib", "win64", BuildFolder);

			/*
			test your path with:
			using System; // Console.WriteLine("");
			Console.WriteLine("... LibrariesPath -> " + LibrariesPath);
			*/

			PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "antlr4-runtime-static.lib")); 
			PublicDefinitions.Add("ANTLR4CPP_STATIC" );
			
		}

		if (isLibrarySupported)
		{
			// Include path
			PrivateIncludePaths.Add( Path.Combine(AntlrPath, "include" ) );

		}

		return isLibrarySupported;
	
	}
}
