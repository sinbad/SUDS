# Comment Lines

Comment lines are ignored by the SUDS importer, and are there just for your
own use.

You add comments to your script by starting the line with a hash/pound (`#`) character:

```yaml
# This is a Comment
# More comments here
NPC: Did you say something?
```

It doesn't matter how you indent the comment line. 

## Comments must be on their OWN line

You ***cannot*** add comments to the end of other types of lines:

```yaml
NPC: Oops  # THIS IS NOT VALID
```

Although other languages support this, SUDS does not right now. 

> The reason is mainly that we reserve the end of lines to append
> [localisation](Localisation.md) tags, and supporting both trailing line
> comments and these would over-complicate things. 

## Translator Comments

SUDS also supports using a specialised comment syntax to communicate information
to translators. Take a look at the [Translator Comments](LocalisationTranslatorComments.md)
section for more information.

### See Also:
* [Translator Comments](LocalisationTranslatorComments.md)
* [Speaker Lines](SpeakerLines.md)
* [Choices](ChoiceLines.md)
* [Variables](Variables.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue in UE](RunningDialogue.md)
* [Localisation](Localisation.md)
* [Full Documentation Index](../Index.md)