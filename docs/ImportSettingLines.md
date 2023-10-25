# Import Setting Lines

There are a few features which can be toggled at import time, but which have no
effect at runtime. They typically take the form:

```yaml
[importsetting Name Value]
```

Import settings apply from the line they are encountered in the script, to every 
line afterwards (ignoring indenting). It's most intuitive to use them in the 
[Header Section](Header.md), although you can put them elsewhere if you need to.

## Enabling Choices as Speaker Lines

As described in [Choice Lines As Speaker Lines](ChoiceLines.md#choices-as-speaker-lines),
it's possible to make the importer generate speaker lines for choices, so that
your player speaks them without having to duplicate the text.

```yaml
[importsetting GenerateSpeakerLinesFromChoices true]
```

You can also change the speaker ID that is generated for these lines:

```yaml
[importsetting SpeakerIDForGeneratedLinesFromChoices `Speaker`]
```


---

### See Also:
* [Choices](ChoiceLines.md)
* [Script Reference](ScriptReference.md)