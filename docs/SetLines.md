# Set Lines

When you want to change [variables](Variables.md) from script, you do it with a 
`set` line, for example:

```yaml
[set SomeNumber 9]
```

> Optionally you can use an "`=`" between the variable name and the value if you
> want, but you don't have to.

All variables are considered to be [dialogue scoped variables](Variables.md#dialogue-variables),
unless they are prefixed with `global.`, in which case they are set as 
[global variables](Variables.md#global-variables).

```yaml
# This sets a global variable which persists across all dialogues
[set global.SomeBool true]
```

## Setting Literal Values

Here's how you set variables using each of the [supported types](Variables.md#supported-types)
from simple literals:

### Integers

```yaml
[set SomeInt 5]
```

### Floating point

```yaml
[set SomeFloat 53.835]
```

> Note: do not include any suffix like 'f'
### Boolean

```yaml
[set SomeBool true]
```

> Supported arguments are `true`, `True`, `false` and `False`

### Text

This is for text (FText) you expect to display to the player. 
```yaml
[set SomeText "Hello world"]
```

You can include double quotes inside text by using the escape (`\`) character:
```yaml
[set SomeText "This includes some \"Quoted\" text"]
```


> Note: Literal text is automatically tagged for [localisation](Localisation.md)
> just like text in [speaker lines](SpeakerLines.md) and [choices](ChoiceLines.md).

### Name

Names (FNames) are not localised unlike text, so are good for signalling more descriptive
values to / from code.

```yaml
[set SomeName `HelloWorld`]
```

### Gender

```yaml
[set SomeGender feminine]
```

> The 3 gender options are `masculine`, `feminine` and `neuter`

## Setting Values Using Expressions

Rather than a literal value, you can set a variable based on an
[expression](Expressions.md), which is a potentially compound statement, which 
can reference other variables and perform operations.

See the [Expressions](Expressions.md) section for a complete discussion, but
for example you could do things like this:

```yaml
[set SomeInt = {SomeInt} + 1]
[set SomeBoolean = {AIsTrue} or {BIsTrue}]
[set IsLargeEnough = {SomeInt} > 10]
```

> Note that I've used the '`=`' symbol here; I don't have to, it's optional, but
> I find it easier to read when there are expressions on the right instead of
> literals.

---

### See Also
 
* [Variables](Variables.md)
* [Expressions](Expressions.md)
* [Script Reference](ScriptReference.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)