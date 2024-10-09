# Events

Events are the most direct way to send ad-hoc messages back to the rest of your game.
You can also just set [variables](Variables.md), because code can be notified on
variable changes, but if you want to send back a message with multiple argument
values, events have your back.

An event line might look something like this:

```yaml
[event MyAwesomeEvent 23, `Boop`, false]
```

## Event Name

The first parameter is the event name, in this case "`MyAwesomeEvent`".

Event names can contain:
* Letters (a-z, A-Z)
* Numbers (0-9)
* Underscores (_)
* Periods (.)


## Event Arguments

Optionally you can include *any number* of arguments after the event name, separated
by commas.

Arguments can be literal values, as shown above, or they can be [expressions](Expressions.md),
referencing variables and optionally doing operations on them. For example:

```yaml
[event MyAwesomeEvent {NumCats}, `Boop`, {HasLaserPointer} or {HasTreats}]
```

See the [Variables](Variables.md) and [Expressions](Expressions.md) sections for
more detail.

> Note: Text literals in event arguments are not localised. If you need them to 
> be, consider using a [set line](SetLines.md) which can localise text and passing
> the variable instead. Or, pass another type and generate localised text in code.

## Receiving Events

C++ or Blueprint code can receive events either by
[subscribing](RunningDialogue.md#delegates) to the `OnEvent` delegate hook of a
dialogue, or by being a [Participant](Participants.md) in the dialogue.

---

## See Also

* [Variables](Variables.md)
* [Expressions](Expressions.md)
* [Script Reference](ScriptReference.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
* [Participants](Participants.md)
* [Full Documentation Index](../Index.md)