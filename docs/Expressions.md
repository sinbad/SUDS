# Expressions

Expressions are sequences of [literals](#literals), [variables](#variables) and 
[operators](#operators) which resolve to 
a single value when evaluated. Examples of expressions include:

```yaml
{NumCats} + 2
{IsMeowing} and not {HasBeenFed}
( 23.4 + 5 ) * 2.5
```

Expressions can be used as arguments when [setting variables](SetLines.md), 
when [raising events](EventLines.md) and for [creating conditions](ConditionalLines.md).

## Literals

Literal values are hard-coded values in an expression.

| Type | Unreal Type | Example | Notes |
|------|-------------|-------| -----|
| Integer | int32 | `5` | Whole numbers between -2,147,483,648 and 2,147,483,647.|
| Float | float | `12.5` | Using a decimal point creates single-precision floats. Do not use '`f`' suffix |
| Boolean | bool | `true` | Literals can be `true`, `True`, `false` or `False` |
| Text | FText | `"Hello World"` | Text is enclosed in double-quotes, may be localised (in [set lines](SetLines.md) only)|
| Name | FName | `` `SomeName` `` | Names are enclosed in backticks |
| Gender | ETextGender | `feminine` | Options are `masculine`, `feminine` or `neuter` |

## Variables

You can reference the value of a variable by enclosing it in curly braces (`{}`),
for example 

```
{NumCats}
```

Variables are ***case insensitive***. See [Variables](Variables.md) for more details.

## Operators

A limited set of operators are supported in SUDS. The list below is presented in
order of operator precedence; meaning that unless overridden by [parentheses](#parentheses), 
expressions are resolved by performing the operators in this list in the order 
they are listed.

|Operator Name|Symbols|Supported Types|Type|Notes|
|-----|-------|------|-----|-----|
| Not | `!`<br/> `not`| Boolean| Unary |Turns true into false and vice versa|
| Multiply | `*` |  Integer, Float| Binary | Result is float if any argument is float, otherwise integer|
| Divide | `\` |  Integer, Float| Binary | Result is float if any argument is float, otherwise integer|
| Add        | `+` | Integer, Float| Binary |Result is float if any argument is float, otherwise integer|
| Subtract   | `-` |  Integer, Float| Binary | Result is float if any argument is float, otherwise integer|
| Less Than   | `<` |  Integer, Float| Binary | Result is boolean, arguments must be comparable |
| Less Than Or Equal   | `<=` |  Integer, Float| Binary | Result is boolean, arguments must be comparable |
| Greater Than   | `>` |  Integer, Float| Binary | Result is boolean, arguments must be comparable |
| Greater Than Or Equal   | `>=` |  Integer, Float| Binary | Result is boolean, arguments must be comparable |
| Equal   | `=`<br/>`==` |  Any | Binary | Result is boolean, arguments must be comparable |
| Not Equal   | `!=`<br/>`<>` |  Any | Binary | Result is boolean, arguments must be comparable |
| And   | `&&`<br/>`and` |  Boolean | Binary | Result is boolean, arguments must be boolean |
| Or   | `\|\|`<br/>`or` |  Boolean | Binary | Result is boolean, arguments must be boolean |


> Yes, you can use a single (`=`) or double-equals (`==`) as a comparison operator; 
> SUDS is a simple language and assignment is via [set](SetLines.md) so this is not ambiguous.


### Parentheses

You can alter or make operator ordering more clear by adding parentheses (`()`)
to your expressions. For example, this expression will yield a result 
of 30:

```yaml
3 + 5 * 2
```

Because the default is to resolve the multiply first, then the add. You can 
change that by doing this:

```yaml
( 3 + 5 ) * 2
```

Which would yield a result of 16 instead.

---

### See Also
 
* [Variables](Variables.md)
* [Set Lines](SetLines.md)
* [Script Reference](ScriptReference.md)
* [Full Documentation Index](../Index.md)