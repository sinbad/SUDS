# Conditional Lines

Giving the [player choices](ChoiceLines.md) is one way that dialogue can branch,
another way is by having conditional paths in your scripts, where different paths
are taken based on [variable state](Variables.md).

You do this by using conditional lines. Here's an example:

```yaml
Player: Hello
[if {x} == 1]
    NPC: Reply when x == 1
    [if {y} > 0]
        Player: Player text when x == 1 and y > 0
    [endif]
[elseif {x} > 1]
    NPC: Reply when x > 1
[else]
    NPC: Reply when x is something else
[endif]
```

Conditional lines start or end a block of other lines which only run if the
containing conditions evaluate to `true`. You have the typical programming 
conditionals `if`, `elseif`, and `else`, and only one of those paths will be taken;
`endif` finishes that conditional set.

Conditional blocks can be nested, as shown above. 

Indentation is technically not required and doesn't have any effect on conditionals. 
However it can help for readability to indent each conditional block.

## Conditional Expressions

The tests in `if` and `elseif` lines are [Expressions](Expressions.md). They 
must resolve to a boolean value (if they don't, there will be an error reported
and that branch will never be taken). You can refer to [variables](Variables.md) and 
perform operations, see the [Expressions](Expressions.md) section for more details.

## Conditional Choices

As well as changing the main sequence of a dialogue, conditions can also turn on/off
[choices](ChoiceLines.md):

```yaml
NPC: Hello
    * First choice
        Player: I took the 1.1 choice
[if {y} == 2]
    * Second choice (conditional)
        Player: I took the 1.2 choice
    * Third choice (conditional)
        Player: I took the 1.3 choice
[else]
    * Second Alt Choice
        Player: I took the alt 1.2 choice        
[endif]
    * Common last choice
        Player: I took the 1.4 choice
```

In this case, "First choice" and "Common last choice" are always available, 
but the other 3 are only available if the condition in the surrounding block passes.
When you [run the dialogue](RunningDialogue.md), which choices are available will
be evaluated when the "NPC: Hello" line becomes active.

Since [choices](ChoiceLines.md) rely on consistent indenting, be careful when
putting conditions around choices that you don't end up changing the meaning of
the choice branches accidentally. Ideally keep the choices at the same indent they
would normally be to make it easier to line things up.


---

### See Also
 
* [Variables](Variables.md)
* [Expressions](Expressions.md)
* [Random Lines](RandomLines.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)
