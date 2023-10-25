# Script Reference

This is a complete reference to all the features of a SUDS Script File.
Scripts are the static template for a [runtime dialogue](RunningDialogue.md),
containing all possible routes through the interaction; think of them like
a movie script.

## General Principles

### File Extension

A SUDS script file must have the extension `.sud`. 

### Lines

SUDS scripts are line-based. Each line is self-contained and does one thing. 
You can't split commands over 2 lines, or combine multiple commands into a single line. 

### Leading / trailing whitespace

Whitespace is trimmed from the start/end of all lines.
However indenting can matter when it comes to branching due to [choices](ChoiceLines.md).

### Import Process

Script files are imported into Unreal as SUDS Script assets and an associated
string table (see [Localisation](Localisation.md)). 

![Import Process](img/ImportProcess.png)

The easiest way to do this is simply to save your `.sud` file in your Unreal 
project's Content folder and confirm the auto-import prompt. You can keep making
changes to the `.sud` and UE will re-import it as changes are detected.

### Use The VSCode Extension 

SUDS has a [VSCode extension](vscode.md) which makes it much more pleasant to edit
the `.sud` format, recognised it as a language and syntax highlighting everything.


## Types of script lines

Here are all the types of lines you can include in your scripts:

| Line Type | Summary |
|-----------|---------|
| [Speaker lines](SpeakerLines.md)  | Lines of dialogue |
| [Choice lines](ChoiceLines.md)    | Player choices that can branch dialogue |
| [Goto lines](GotoLines.md)        | Jump dialogue execution to another line |
| [Gosub & Return lines](GosubLines.md)        | Jump to another line, then return back |
| [Set lines](SetLines.md)          | Set variable values |
| [Conditional lines](ConditionalLines.md)  | Include/exclude parts of the dialogue based on state |
| [Event lines](EventLines.md)              | Raise events to be received by code |
| [Comment Lines](CommentLines.md) | Ignored by the importer |
| [Import Setting Lines](ImportSettingLines.md) | Change behaviour during import |

## Script Sequence

A script is generally run from top to bottom, starting at the top unless told to start from
a [label](GotoLines.md#label-lines). Lines are executed one after the other until 
a [Speaker line](SpeakerLines.md) is hit, at which point the dialogue will pause 
and wait to be told to continue; potentially via a player choice, or simply just a continue prompt.

A few things can alter this simple top-to-bottom sequence:

* [Headers](Header.md): Lines contained in the [header](Header.md) section of
    the script are executed when the dialogue is created.
* [Choices](ChoiceLines.md): choices represent player decisions which branch the dialogue.
    Everything indented under a [choice line](ChoiceLines.md) is only executed if 
    that choice is picked. 
* [Gotos](GotoLines.md): goto lines jump the dialogue execution to either a 
    [label](GotoLines.md#label-lines), or to `end`, which terminates the dialogue.
* [Conditionals](ConditionalLines.md): conditional lines include or exclude sections of the dialogue
    based on [variable state](Variables.md).

There are also [fallthrough rules](ChoiceLines.md#fallthrough) which control what
happens when you reach the bottom of an indented section (under a choice).

---

### See Also

* [Visual Studio Extension](vscode.md)
* [Variables](Variables.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)
* [Frequently Asked Questions](docs/FAQ.md)