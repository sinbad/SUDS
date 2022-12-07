# Choice Lines

When speaker lines follow each other, the dialogue follows a linear sequence. 
It's assumed that you'll provide a "continue" button in your game to advance to 
the next line. 

When you want the player to make a decision and branch the dialogue, or even if 
you just want some text associated with a single continuation, then you need a *choice line*.

Choice lines begin with an asterisk, and must be contained on the rest of the line.
In the example below, there are 3 choices following the first dialogue line:

```yaml
Vagabond: Well met, fellow traveller!
  * Er, hi?
    Vagabond: Verily, 'tis wondrous to see such a fine fellow on the road this morn!
  * (Keep quiet)
    Vagabond: What, cat got your tongue?
  * Jog on, mate
    Vagabond: Well, really! Good day then sir!
Narrator: (The Vagabond leaves)
```

Underneath each choice, you put the follow-on lines that will be taken if this choice
is selected. These lines ***must be indented*** to at least one character inward of
the asterisk itself (I recommend 2 or 4).

Any type of line can be included in these indented follow-on sections, including
other choices (although there must be a speaker line in between, see below).
You can nest as far as you wish, although it may become unwieldy eventually,
and you'd be better to use a [goto](GotoLines.md).

```yaml
NPC: What's up?
  * What's going on?
    NPC: What, generally or was there something specific?
      * Oh, er generally I guess
        NPC: Not much.
      * How's the weather?
        NPC: Ehh, not bad. Probably rain later.
          * Rain? Oh no.
            NPC: It is April after all
          * I quite like rain
            NPC: It's fine until it gets into your shoes.
      * Did you see that ludicrous display last night?
        NPC:  Thing about Arsenal, they always try and walk it in.
  * Oh, never mind
    NPC: OK bye
```

This is where you can see why the indenting is important. All the asterisks at
the same level of indenting are part of the same set of choices, while everything
indented further is part of one of the choice branches, which can be nested. 

This example creates a dialogue tree that could be visualised like this:

```mermaid
flowchart TD
  A(What's up?) -- What's going on? --> B(What, generally or was there something specific?);
  A -- Oh, never mind --> C(OK bye);
  B -- Oh, er generally I guess --> D(Not much.);
  B -- How's the weather? --> E(Ehh, not bad. Probably rain later.);
  E -- Rain? Oh no. --> F(It is April after all);
  E -- I quite like rain --> G(It's fine until it gets into your shoes.);
  B -- Did you see that ludicrous display last night? --> H(Thing about Arsenal, they always try and walk it in.);
```

You might be wondering what happens when the dialogue reaches the end of a
choice section, if there's no [goto](GotoLines.md) line sending it somewhere else. 
That's covered later in the [fallthrough section](#fallthrough).

## Choices And Speaker Lines

Choices must ***always*** follow on from a [speaker line](SpeakerLines.md), they can't
exist on their own. Usually choices will be directly underneath a speaker line, and for 
readability we recommend some kind of indent. However, choices can also follow on from a speaker line via a 
[goto](GotoLines.md):

```yaml
Shopkeep: What will it be?
  :choices
  * What's that knobbly thing?
    Shopkeep: That would be potato, sir. 
    [goto choices]
  * One jelly baby if I may, ideally green
    Shopkeep: One??
    ...
```

In this case, note that the `:choices` label is directly above the choices,
rather than above the speaker line. This means that when we `[goto choices]`
just after the "That would be a potato, sir" line, the choices are used as 
continuations from that line, and we don't have to repeat the original "What will it be"
line that started off the sequence. It's good for looping dialogue, letting you
re-present the same list of choices but in the context of the last dialogue line.

## Variable Substitution

In the text sections of a choice, you can substitute the values of [variables](Variables.md)
using curly braces (`{}`), something like this:

```yaml
  * Did you see {SuspectName} on the night of July 3rd?
```

For more information, see [Text Markup](TextMarkup.md).

## Fallthrough

What happens when you reach the end of a choice branch, and there's no [goto](GotoLines.md)
sending the execution somewhere else?

In that case dialogue execution "falls through" to the next line which is outdented
more than the current line, and is on the same "choice path" (i.e. it doesn't
cross to a place where you'd have had to pick a different choice to get there).

Let's take the previous example and add some additional lines as intermediate fallthrough points
(the previous example would have fallen through to the end in every case).

```yaml
NPC: What's up?
  * What's going on?
    NPC: What, generally or was there something specific?
      * Oh, er generally I guess
        NPC: Not much.
      * How's the weather?
        NPC: Ehh, not bad. Probably rain later.
          * Rain? Oh no.
            NPC: It is April after all
          * I quite like rain
            NPC: It's fine until it gets into your shoes.
        NPC: Still, they say it's going to clear up next week
      * Did you see that ludicrous display last night?
        NPC:  Thing about Arsenal, they always try and walk it in.
    Player: Well, that's all I wanted to ask.
  * Oh, never mind
    NPC: OK bye
Player: Later!
```

Let's update that flowchart to show where the fallthroughs happen:

```mermaid
flowchart TD
  A(What's up?) -- What's going on? --> B(What, generally or was there something specific?);
  A -- Oh, never mind --> C(OK bye);
  C -.-> Z(Later!)
  B -- Oh, er generally I guess --> D(Not much.);
  B -- How's the weather? --> E(Ehh, not bad. Probably rain later.);
  E -- Rain? Oh no. --> F(It is April after all);
  E -- I quite like rain --> G(It's fine until it gets into your shoes.);
  B -- Did you see that ludicrous display last night? --> H(Thing about Arsenal, they always try and walk it in.);
  F -.-> Y(Still, they say it's going to clear up next week);
  H -.-> X(Well, that's all I wanted to ask);
  D -.-> X;
  G -.-> Y;
  Y -.-> X;
  X -.-> Z;
```

The fallthrough points are shown by dotted lines. To follow how this works, 
let's take an inner snippet; Let's say we're on the line indicated by the arrow, "It is April after all":

```yaml
        NPC: Ehh, not bad. Probably rain later.
          * Rain? Oh no.
----->      NPC: It is April after all
          * I quite like rain
            NPC: It's fine until it gets into your shoes.        
        NPC: Still, they say it's going to clear up next week
```

When we continue, we find there's no more lines under this choice, so we fall
through to the next outdented line that is NOT on a different choice path.
So even though "I quite like rain" is outdented, it's a different choice path, 
so we skip it. 

The next line that's outdented but on the same choice path is the speaker line
"Still, they say it's going to clear up next week".

```yaml
        NPC: Ehh, not bad. Probably rain later.
          * Rain? Oh no.
   .-----   NPC: It is April after all
   |      * I quite like rain
   |        NPC: It's fine until it gets into your shoes.        
   `->  NPC: Still, they say it's going to clear up next week
```

You can fallthrough to lines other than speaker lines, including [Set lines](SetLines.md), 
[Goto lines](GotoLines.md), [Event lines](EventLines.md). 

### Fallthrough to choices

You cannot fallthrough directly to other choices. This is ***not valid*** for example: 

```yaml
NPC: Some text
  * Option 1
    NPC: Let's say I want to fall through from here to the choice below


  * Intended fallthrough choice
  ...
```

This is because it's ambiguous - that choice is actually a different
branch of the same choice list, not something you can fall through from.
You can fix this by introducing another line between them; either another
speaker line:

```yaml
NPC: Some text
  * Option 1
    NPC: Let's say I want to fall through from here to the choice below

NPC: New speaker line which is a valid fallthrough target
  * Intended fallthrough choice
  ...
```

Or, if you didn't want to add an extra line you can make it an explicit [goto](GotoLines.md)
instead of a fallthrough:

```yaml
NPC: Some text
  * Option 1
    NPC: Let's say I want to fall through from here to the choice below
    [goto secondchoice]

:secondchoice
  * Intended fallthrough choice
  ...
```

The addition of the label and goto splits the choice list up and removes the
ambiguity, while still allowing you to go directly from the speaker line inside
the "Option 1" choice to the later choice list, without an intervening speaker line.

Also, sometimes it can just make your script clearer if you use explicit
[gotos](GotoLines.md) instead of relying on fallthrough behaviour.


### Conditionals and fallthroughs

Lines in one [Conditional Block](ConditionalLines.md) can only fall through to 
lines which are in the same block, or in a containing block (including outside 
any conditional).

---

### See Also:
* [Text Markup](TextMarkup.md)
* [Speaker lines](SpeakerLines.md)
* [Running Dialogue in UE](RunningDialogue.md)
* [Variables](Variables.md)
* [Localisation](Localisation.md)
* [Full Documentation Index](../Index.md)