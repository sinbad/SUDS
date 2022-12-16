# Gosub / Return Lines

Gosub statements operate just like [Goto](GotoLines.md), except that execution
will return to the point of the Gosub statement afterwards, on hitting a `return` line.


```yaml
[gosub SomeLabel]
```

If you prefer, you can also use `go sub` instead of `gosub`. 

In this case we'd jump to a [label](GotoLines.md#label-lines) called `SomeOtherLabel`, which are
the same as [labels used for gotos](GotoLines.md#label-lines).

Execution then proceeds until the dialogue sees a `return` line:

```yaml
[return]
```

The `return` line sends execution back to the line *after* the last `gosub` line
that was executed. For example:

```yaml
Player: Hello there
[gosub Greeting]
Player: Thanks!
[goto end]


:Greeting
NPC: Hello to you too!
[return]
```

This is a contrived example, but the sequence would be:

```yaml
Player: Hello there
NPC: Hello to you too!
Player: Thanks!
```

The point of this is that you could use the "Greeting" sub-dialogue in multiple 
places in your dialogue, but always return back to the place you invoked it from.
This lets you put the same exchange in multiple places in your dialogue without
duplicating it, which can be very useful.

## Nested gosubs

It's fine to have another call to `gosub` inside a bit of sub-dialogue that was
itself reached through a `gosub`. SUDS keeps a stack of all the `gosub` lines which
have been executed to get to where you are, and the `return` statement always returns to the
line after the *last* `gosub` executed in that stack, before removing it and
making the previous one the return point.


## Other gosub rules

`gosub` and `return` lines adhere to the same rules as [Goto Lines](GotoLines.md).
Essentially they both behave just like `goto`, just with additional context.



---

### See Also

* [Goto Lines](GotoLines.md)
* [Variables](Variables.md)
* [Script Reference](ScriptReference.md)
* [Localisation](Localisation.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)


