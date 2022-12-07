# Steve's Unreal Dialogue System (SUDS)

![SUDS Logo](Resources/Icon128.png)

## What Is It?

SUDS is a plugin for [Unreal Engine 5](https://unrealengine.com/) which allows you
to run dialogues in your game based on a script that you write in a text 
file.

Scripts look something like this:

![Sample SUDS Script](docs/img/samplescript.png)

> Note: this is an image so that you can see the nice syntax highlighting provided by the
> [SUDS VSCode plugin](https://marketplace.visualstudio.com/items?itemName=sstreeting.suds-code).

SUDS has many features, including:

* Dialogue flow is expressed in a text file for a focussed writing experience
* Multi-line speech support
* Player choices with unlimited embedded responses
* Flow control, conditional branching
* Persistent variable state
* Send events back to code/blueprints with any number of arguments
* Supports all the [text replacement and formatting features](https://docs.unrealengine.com/4.26/en-US/ProductionPipelines/Localization/Formatting/#textformatting) in the same way Unreal does
* Localisation support
* Easy integration with save games ([SPUD](https://github.com/sinbad/SPUD) or otherwise)

> SUDS was inspired by [Ink](https://www.inklestudios.com/ink/) and
> [YarnSpinner](https://yarnspinner.dev/), but I started from scratch and 
> designed it specifically to integrate well with Unreal Engine. It also has a 
> number of differences to those systems, based on my own preferences and the 
> needs of my own project.

## Getting Started

* [Installation](docs/Installation.md)
* [Your First SUD Script](docs/MyFirstSUDScript.md)
* [Running Dialogue in UE](docs/RunningDialogue.md)
* [Script reference](docs/ScriptReference.md)
* [Full Documentation Index](docs/Index.md)



