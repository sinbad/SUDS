# Speaker Lines

Speaker lines are the primary building block of a [SUDS script](ScriptReference.md).
They represent a single line of dialogue, associated with a [speaker](#speakers).

```yaml
Beatrice: Against my will, I am sent to bid you come in to dinner.
```

You can also create multi-line speaker lines by using continuation lines:

```yaml
Beatrice:  I took no more pains for those thanks than you take pains to thank me. 
           If it had been painful, I would not have come.
```

This is still a single "line" of dialogue, issued all at once, but contains an
embedded carriage return. A new speaker line is not started unless the 
[speaker](#speakers) prefix occurs again. Because leading and trailing whitespace are removed, 
you can align this continuation however you like.

The format is more formally:

```yaml
SpeakerID: Line
OptionalContinuation*
```
|||
|---------|---------|
| SpeakerID | Identifier for the speaker; also its [display name](#speaker-display-names) unless otherwise defined. Cannot contain whitespace. |
| Line | The line of dialogue text. Any characters allowed, can include [variables and rich text formatting](#variables-and-text-formatting).|
| OptionalContinuation | One or more continuation lines which append their contents to the previous speaker line, with the carriage return preserved (but leading/trailing whitespace removed)|


## Speakers

Speakers are identified by the prefix at the start of a speaker line, with a colon
(`:`) separating it from the text of the line. Speakers IDs ***cannot contain whitespace***.

By default, the speaker ID is also used as the display name to the player
(calling `GetSpeakerID` and `GetSpeakerDisplayName` on the runtime dialogue returns
the same thing). 

Alternatively you can define a [display name](#speaker-display-names) for a
speaker separately.

### Speaker Display Names

There are several reasons why you might want a separate display name for a speaker
instead of using the Speaker ID directly:

* Whitespace:
  Speaker ID's cannot have whitespace in them, so if you want your character's name
  to include a space, it needs a separate display name
* Brevity: Maybe you just want to type a shorter name most of the time
* Localisation: You might want to give your characters language-specific names.

Speaker display names are implemented using the [variable system](Variables.md).
If you set a variable called `SpeakerName.SpeakerID` (replacing `SpeakerID` with
the matching speaker identifier) to a text value, that will be used as the display name
for that speaker from then on.

```yaml
[set SpeakerName.Percy "Lord Percy Percy, Heir to the Duchy of Northumberland"]
```

You might want to set speaker display names in the [header](Header.md) of your script,
ensuring they're always set regardless of where you start the dialogue. Alternatively,
because [Set lines](SetLines.md) can be placed anywhere, you can change the 
display name of your speakers between lines if you want, for example to have a 
reveal of a character's name later in the dialogue.

> Because text values in [Set lines](SetLines.md) are automatically tagged for
> [localisation](Localisation.md), you can set the name in the primary language
> here and localise it later.


## Variables and Text Formatting

In the text sections of a speaker line, you can substitute the values of [variables](Variables.md), 
and add rich text formatting markup. 

As a quick guide, add variables using curly braces (`{}`), and rich text formatting 
using angle brackets (`<>`), something like this:

```yaml
Urchin: Yeah I saw the <red>thief</> mister, he ran down {DiversionaryStreetName}!
```

For more information, see [Text Markup](TextMarkup.md).

---

### See Also:
* [Text Markup](TextMarkup.md)
* [Choices](ChoiceLines.md)
* [Variables](Variables.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue in UE](RunningDialogue.md)
* [Localisation](Localisation.md)
* [Full Documentation Index](../Index.md)