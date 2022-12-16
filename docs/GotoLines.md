# Goto Lines

Goto lines simply jump the execution of the dialogue to another
line, and dialogue continues from there.

> If you need to come back to this location again afterwards, see the [Gosub](GosubLines.md) statement.

```yaml
[goto SomeOtherPlace]
```

If you prefer, you can also use `go to` instead of `goto`. 

In this case we'd jump to a label called `SomeOtherPlace`. Of course, you need
to define the location of this label, which you do with...

## Label Lines

Label lines start with a colon (`:`), and any sequence of alphanumeric characters
or underscores (importantly, no dashes or other symbols).

```yaml
:SomeOtherPlace
```

Labels are ***case insensitive*** so you could just as validly jump to the above
label using:

```yaml
[goto someotherplace]
```

## Ending a dialogue

Using `end` as a target for a goto always terminates the dialogue:

```yaml
[goto end]
```

Unsurprisingly, you cannot define your own label called `end` for this reason.

## Gotos And Labels Between Speaker Lines And Choices

When we talked about [choices and their relationship to speaker lines](ChoiceLines.md#choices-and-speaker-lines)
we mentioned that choices have to follow a speaker line, because they represent
the branching paths from that line. 

It's valid to use a `goto` to make this connection though, for example:

```yaml
NPC: Well hello
    :choices
    * Choice 1
      NPC: Some response
      [goto choices]
    * Choice 2
      NPC: Some other response
      [goto choices]
    * Leave
      [goto end]
```

Because the label is between the original entry text "Well hello" and the choices, 
you can send other points of the dialogue back to this point after a new speaker
line to re-use the same choices as a follow on from those lines, including doing 
looping as above, without repeating that "Well hello" line (which would happen if
you put the `:choices` label above it).

---

### See Also

* [Gosub Lines](GosubLines.md)
* [Variables](Variables.md)
* [Script Reference](ScriptReference.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)
