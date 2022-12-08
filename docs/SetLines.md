# Set Lines

When you want to change [variables](Variables.md) from script, you do it with a 
`set` line, for example:

```yaml
[set SomeNumber 9]
```

> Optionally you can use an "`=`" between the variable name and the value if you
> want, but you don't have to.

## Setting Literal Values

Here's how you set variables using each of the [supported types](Variables.md#supported-types)
from simple literals:

### Integers

Underlying type: int32
```yaml
[set SomeInt 5]
```

### Floating point

Underlying type: float

```yaml
[set SomeFloat 53.835]
```

> Note: do not include any suffix like 'f'
### Boolean

Underlying type: bool

```yaml
[set SomeBool true]
```

> Supported arguments are `true`, `True`, `false` and `False`

### Text

Underlying type: FText

This is for text you expect to display to the player. 
```yaml
[set SomeText "Hello world"]
```

> Note: Literal text is automatically tagged for [localisation](Localisation.md)
> just like text in [speaker lines](SpeakerLines.md) and [choices](ChoiceLines.md).

### Name

Underlying type: FName

Names are not localised, unlike text, so are good for signalling more descriptive
values to / from code.

```yaml
[set SomeName `HelloWorld`]
```

### Gender

Underlying type: ETextGender

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
* [Running Dialogue in UE](RunningDialogue.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
