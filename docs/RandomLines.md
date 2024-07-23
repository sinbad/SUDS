# Random Lines

Sometimes rather than explicitly choosing a path as in [Conditional Lines](ConditionalLines.md),
you want to select a random path to mix things up. Here's how you might do this:

```yaml
Player: Hello
[random]
    NPC: Hello!
[or]
    NPC: Greetings!
[or]
    NPC: Hey, I guess.
[endrandom]
```

You begin a random block with a `[random]` line. The first block underneath this
is the first random option. Every time the `[random]` line is encountered
in the dialogue, an internal random roll will determine which option is picked.

Every time you want to introduce another random option, use an `[or]` line
to separate it. This will become an equally weighted option. 

After the final random option of the group, end it with an `[endrandom]` line.

Indenting technically doesn't matter because the random/or/endrandom lines
fully demarcate the groupings; but you will find it easier to follow if you 
match the indents.

### See Also
 
* [Variables](Variables.md)
* [Expressions](Expressions.md)
* [Conditional Lines](ConditionalLines.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)
