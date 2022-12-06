# Writing Your First Script

## Scripts and Dialogues

SUDS has two primary concepts: 

* **Script**: This is a sequence of dialogue lines, including all the possible routes through that dialogue. Think of it like a movie script.
* **Dialogue**: This is a runtime instance of a Script. The Dialogue essentially
    steps through the script based on choices the player makes. 

For this section, we're concerned only with writing the **Script**. We'll cover
[how to run Dialogues](RunningDialogue.md) later.

## Importing Script Source into Unreal as a Script Asset

In SUDS you write the script as a text file, with the extension `.sud`. We'll 
call this the *SUDS Script Source File*.

Whenever you save a file of type `.sud` inside your Unreal Engine content folder, 
the SUDS custom importer will import that source file into a *SUDS Script Asset*.
This is the same as if you put a PNG file in your content directory and imported it
into a Texture Asset. 

![Import Process](img/ImportProcess.png)

> In fact you get both a *Script Asset* and a *String Table*, which helps with [localisation](Localisation.md).

You'll always be writing your dialogue in SUDS Script Source Files in a text editor,
but at runtime you'll be using the SUDS Script Assets. They're small and efficient
because all the work to interpret them has already been done in the importer.

The [VSCode extension](vscode.md) means "SUDS" is a recognised language and will be
automatically used whenever you save a file with the `.sud` extension.
It's **highly recommended** to use VSCode with this extension as your script editing tool.

## Speaker lines

The basic building block of a script is the speaker line.