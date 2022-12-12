# Installation

* [Getting the library](#getting-the-library)
* [Using In C++](#using-in-c)
* [Installing the VSCode Extension](vscode.md)

## Getting The Library

### Cloning

The best way is to clone this repository as a submodule; that way you can contribute
pull requests if you want. The project should be placed in your project's Plugins folder.

```
> cd YourProject
> git submodule add https://github.com/sinbad/SUDS Plugins/SUDS
> git add .gitmodules
> git commit
```

Alternatively you can download the ZIP of this repo and place it in 
`YourProject/Plugins/SUDS`.

## Using in C++

Edit YourProject.Build.cs and do something similar to this:

```csharp
using System.IO;
using UnrealBuildTool;

public class YourProject : ModuleRules
{
	private string PluginsPath
	{
		get { return Path.GetFullPath( Path.Combine( ModuleDirectory, "../../Plugins/" ) ); }
	}
	
	protected void AddSUDS() {
		// Linker
		PrivateDependencyModuleNames.AddRange(new string[] { "SUDS" });
		// Headers
		PublicIncludePaths.Add(Path.Combine( PluginsPath, "SUDS", "Source", "SUDS", "Public"));
	}

	public SUDSExamples(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		
		AddSUDS();
	}
}
```
---
### Next:
* [Your First SUDS Script](MyFirstSUDScript.md)
* [Full Documentation Index](../Index.md)