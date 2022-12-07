# Text Markup

Inside the text for [speaker lines](SpeakerLines.md) and [choices](ChoiceLines.md),
you can include special markup to [substitute variables](#variable-substitution) 
and to [format the text](#rich-text-formatting). 

> Note: SUDS uses the same features and syntax as Unreal's own 
> [text formatting system](https://docs.unrealengine.com/5.1/en-US/ProductionPipelines/Localization/Formatting/#textformatting),
> although we only support named variables, not indexes. 

## Variable Substitution

To substitute a variable into text, use curly braces (`{}`):

```yaml
Shopkeep: So, what'll it be?
  * Take {ItemName}
    Shopkeep: Ah, the {ItemName} eh? Classic.
```

All types of variables can be substituted like this

### Plurals

Sometimes you need to modify the text based on how many of a thing is being discussed. 
Here we support the Unreal [plural forms](https://docs.unrealengine.com/5.1/en-US/text-localization-in-unreal-engine/#pluralforms)
which can be used to emit different text based on a variable number.

You do this by appending a pipe character (`|`) after the variable and adding `plural(...)`,
as shown below:

```yaml
Child: There {NumCats}|plural(one=is,other=are) {NumCats} {NumCats}|plural(one=cat,other=cats)
```

This results in "There is 1 cat" or "There are 7 cats" based on the plurality
of the number. You can use `zero`, `one`, `two`, `few`, `many`, and `other`
as defined in the [CLDR](https://cldr.unicode.org/index/cldr-spec/plural-rules) 
for a given language.

### Genders

If you need to gender your text in some way, you can use the Unreal [gender forms](https://docs.unrealengine.com/5.1/en-US/text-localization-in-unreal-engine/#genderforms)
to alter text:

```yaml
Worker: Oh, that's {Name}, {Gender}|gender(he,she,they) {Gender}|gender(has,has,have) the best stapler in the office.
```

This requires that `{Gender}` is a [variable](Variables.md) of type gender, 
which can be `masculine`, `feminine`, or `neuter` as per the Unreal `ETextGender` type.

## Rich Text Formatting

All text in SUDS supports being marked up for use in a [Rich Text Block](https://docs.unrealengine.com/5.1/en-US/umg-rich-text-blocks-in-unreal-engine/). This requires that you set up 
a datatable of styles - that's outside the scope of this documentation, but
refer to the [official documentation](https://docs.unrealengine.com/5.1/en-US/umg-rich-text-blocks-in-unreal-engine/)
for details.

If you've set this up you can include rich text tags to add variable styles, 
images, and even [animated text](https://www.stevestreeting.com/2022/09/14/text-animation-effects-in-unreal-engine/),

```yaml
Buttercup: We'll <Shake>never</> survive!
Wesley: <Bold>Nonsense.</> You’re only saying that because no one ever has.
```

---
### See Also:
* [Speaker Lines](SpeakerLines.md)
* [Choice Lines](ChoiceLines.md)
* [Script Reference](ScriptReference.md)
* [Running Dialogue in UE](RunningDialogue.md)
* [Variables](Variables.md)
* [Localisation](Localisation.md)
* [Full Documentation Index](../Index.md)