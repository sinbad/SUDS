# Headers

When you [run a dialogue](RunningDialogue.md), you can start it from any [Label ](GotoLines.md#label-lines)
rather than from the top, if you want. 

But what if you have a set of initial [variable state](Variables.md) that you want 
to always be set up, regardless of where the dialogue starts from?

Headers exist to solve this problem. Headers must be declared at the top of a
[script](ScriptReference.md), and are enclosed by lines with 3 equal signs:

```yaml
===
[set NumberOfWishes 3]
===
```

Only [set lines](SetLines.md) are allowed in headers. 

Headers are run when dialogue is ***created***, before you call `Start`. This is to 
allow you to override variables that are set in the header before calling `Start`
if you wish.

---

### See Also
 
* [Set Lines](SetLines.md)
* [Variables](Variables.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue](RunningDialogue.md)
* [Full Documentation Index](../Index.md)