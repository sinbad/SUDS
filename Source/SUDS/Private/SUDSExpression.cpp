// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSExpression.h"

#include "SUDSLibrary.h"
#include "Misc/DefaultValueHelper.h"
#include "Internationalization/Regex.h"

bool FSUDSExpression::ParseFromString(const FString& Expression, FString* OutParseError)
{
	// Assume invalid until we've parsed something
	bIsValid = false;
	Queue.Empty();
	VariableNames.Empty();
	SourceString = Expression;
	
	// Shunting-yard algorithm
	// Thanks to Nathan Reed https://www.reedbeta.com/blog/the-shunting-yard-algorithm/
	// We take a natural string, parse it and then turn it into a queue of tokens (operators and operands)
	// expressed in Reverse Polish Notation, which can be easily executed later
	// Variables are not resolved at this point, only at execution time.
	
	// Split into individual tokens; sections of the regex are:
	// - {Variable}
	// - Literal numbers (with or without decimal point, with or without preceding negation)
	// - Arithmetic operators & parentheses
	// - Boolean operators & comparisons
	// - Predefined constants (Masculine, feminine, true, false etc)
	// - Quoted strings "string"
	//   - Including ignoring escaped double quotes
	// - Quoted names `name`
	const FRegexPattern Pattern(TEXT("(\\{[\\w\\.]+\\}|-?\\d+(?:\\.\\d*)?|[-+*\\/\\(\\)]|and|&&|\\|\\||or|not|\\<\\>|!=|!|\\<=?|\\>=?|==?|[mM]asculine|[fF]eminine|[nN]euter|[tT]rue|[fF]alse|\"(?:[^\"\\\\]|\\\\.)*\"|`([^`]*)`)"));
	FRegexMatcher Regex(Pattern, Expression);
	// Stacks that we use to construct
	TArray<ESUDSExpressionItemType> OperatorStack;
	bool bParsedSomething = false;
	bool bErrors = false;
	while (Regex.FindNext())
	{
		FString Str = Regex.GetCaptureGroup(1);
		ESUDSExpressionItemType OpType = ParseOperator(Str);
		if (OpType != ESUDSExpressionItemType::Null)
		{
			bParsedSomething = true;

			if (OpType == ESUDSExpressionItemType::LParens)
			{
				OperatorStack.Push(OpType);
			}
			else if (OpType == ESUDSExpressionItemType::RParens)
			{
				if (OperatorStack.IsEmpty())
				{
					if (OutParseError)
						*OutParseError = TEXT("Mismatched parentheses");
					bErrors = true;
					break;
				}
					
				while (OperatorStack.Num() > 0 && OperatorStack.Top() != ESUDSExpressionItemType::LParens)
				{
					Queue.Add(FSUDSExpressionItem(OperatorStack.Pop()));
				}
				if (OperatorStack.IsEmpty())
				{
					if (OutParseError)
						*OutParseError = TEXT("Mismatched parentheses");
					bErrors = true;
					break;
				}
				else
				{
					// Discard left parens
					OperatorStack.Pop();
				}
				
			}
			else
			{
				// All operators are left-associative except not
				const bool bLeftAssociative = OpType != ESUDSExpressionItemType::Not;
				// Valid operator
				// Apply anything on the operator stack which is higher / equal precedence
				while (OperatorStack.Num() > 0 &&
					// higher precedence applied now, and equal precedence if left-associative
					(static_cast<int>(OperatorStack.Top()) < static_cast<int>(OpType) ||
					(static_cast<int>(OperatorStack.Top()) <= static_cast<int>(OpType)	&& bLeftAssociative)))
				{
					Queue.Add(FSUDSExpressionItem(OperatorStack.Pop()));
				}

				OperatorStack.Push(OpType);
			}
		}
		else
		{
			// Attempt to parse operand
			FSUDSValue Operand;
			if (ParseOperand(Str, Operand))
			{
				bParsedSomething = true;
				Queue.Add(FSUDSExpressionItem(Operand));
			}
			else
			{
				if (OutParseError)
					*OutParseError = FString::Printf(TEXT("Unrecognised token %s"), *Str);
				bErrors = true;
			}
		}
	}
	// finish up
	while (OperatorStack.Num() > 0)
	{
		if (OperatorStack.Top() == ESUDSExpressionItemType::LParens ||
			OperatorStack.Top() == ESUDSExpressionItemType::RParens)
		{
			bErrors = true;
			if (OutParseError)
				*OutParseError = TEXT("Mismatched parentheses");
			break;
		}

		Queue.Add(FSUDSExpressionItem(OperatorStack.Pop()));
	}

	if (!Validate() ||
		(Expression.Len() > 0 && Queue.IsEmpty())) // Empty expressions validate correctly, but if there was text incoming that resolved to nothing, this is an error
	{
		bErrors = true;
		if (OutParseError)
			*OutParseError = FString::Printf(TEXT("Bad expression '%s'"), *Expression);
	}

	bIsValid = bParsedSomething && !bErrors;

	// Build list of variables
	if (bIsValid)
	{
		for (auto& Item : Queue)
		{
			if (Item.IsOperand() && Item.GetOperandValue().IsVariable())
			{
				VariableNames.AddUnique(Item.GetOperandValue().GetVariableNameValue());
			}
		}

	}

	return bIsValid;
}


ESUDSExpressionItemType FSUDSExpression::ParseOperator(const FString& OpStr)
{
	if (OpStr == "+")
		return ESUDSExpressionItemType::Add;
	if (OpStr == "-")
		return ESUDSExpressionItemType::Subtract;
	if (OpStr == "*")
		return ESUDSExpressionItemType::Multiply;
	if (OpStr == "/")
		return ESUDSExpressionItemType::Divide;
	if (OpStr == "and" || OpStr == "&&")
		return ESUDSExpressionItemType::And;
	if (OpStr == "or" || OpStr == "||")
		return ESUDSExpressionItemType::Or;
	if (OpStr == "not" || OpStr == "!")
		return ESUDSExpressionItemType::Not;
	if (OpStr == "==" || OpStr == "=")
		return ESUDSExpressionItemType::Equal;
	if (OpStr == ">=")
		return ESUDSExpressionItemType::GreaterEqual;
	if (OpStr == ">")
		return ESUDSExpressionItemType::Greater;
	if (OpStr == "<=")
		return ESUDSExpressionItemType::LessEqual;
	if (OpStr == "<")
		return ESUDSExpressionItemType::Less;
	if (OpStr == "<>" || OpStr == "!=")
		return ESUDSExpressionItemType::NotEqual;
	if (OpStr == "(")
		return ESUDSExpressionItemType::LParens;
	if (OpStr == ")")
		return ESUDSExpressionItemType::RParens;

	return ESUDSExpressionItemType::Null;
}

bool FSUDSExpression::ParseOperand(const FString& ValueStr, FSUDSValue& OutVal)
{
	// Try Boolean first since only 2 options
	{
		if (ValueStr.Compare("true", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(true);
			return true;
		}
		if (ValueStr.Compare("false", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(false);
			return true;
		}
	}
	// Try gender
	{
		if (ValueStr.Compare("masculine", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Masculine);
			return true;
		}
		if (ValueStr.Compare("feminine", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Feminine);
			return true;
		}
		if (ValueStr.Compare("neuter", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Neuter);
			return true;
		}
	}
	// Try quoted text (will be localised later in asset conversion)
	{
		const FRegexPattern Pattern(TEXT("^\"((?:[^\"\\\\]|\\\\.)*)\"$"));
		FRegexMatcher Regex(Pattern, ValueStr);
		if (Regex.FindNext())
		{
			FString Val = Regex.GetCaptureGroup(1);
			// Consolidate any escaped double quotes into just quotes
			Val.ReplaceInline(TEXT("\\\""), TEXT("\""));
			OutVal = FSUDSValue(FText::FromString(Val));
			return true;
		}
	}
	// Try FName
	{
		const FRegexPattern Pattern(TEXT("^`([^`]*)`$"));
		FRegexMatcher Regex(Pattern, ValueStr);
		if (Regex.FindNext())
		{
			const FString Name = Regex.GetCaptureGroup(1);
			OutVal = FSUDSValue(FName(Name), false);
			return true;
		}
	}
	// Try variable name
	{
		const FRegexPattern Pattern(TEXT("^\\{([^\\}]*)\\}$"));
		FRegexMatcher Regex(Pattern, ValueStr);
		if (Regex.FindNext())
		{
			const FName VariableName(Regex.GetCaptureGroup(1));
			OutVal = FSUDSValue(VariableName, true);
			return true;
		}
	}
	// Try Numbers
	{
		float FloatVal;
		int IntVal;
		// look for int first; anything with a decimal point will fail
		if (FDefaultValueHelper::ParseInt(ValueStr, IntVal))
		{
			OutVal = FSUDSValue(IntVal);	
			return true;
		}
		if (FDefaultValueHelper::ParseFloat(ValueStr, FloatVal))
		{
			OutVal = FSUDSValue(FloatVal);	
			return true;
		}
	}

	return false;
	
}

bool FSUDSExpression::Validate()
{
	// Empty expressions are always valid, mean "true"
	if (Queue.IsEmpty())
		return true;

	// Same algorithm as Execute, we just don't execute
	TArray<FSUDSExpressionItem> EvalStack;
	const TMap<FName, FSUDSValue> TempVariables;
	for (auto& Item : Queue)
	{
		if (Item.IsOperator())
		{
			FSUDSExpressionItem Arg1, Arg2;
			if (Item.IsBinaryOperator())
			{
				if (EvalStack.IsEmpty())
					return false;
				Arg2 = EvalStack.Pop();
			}
			if (EvalStack.IsEmpty())
				return false;
			Arg1 = EvalStack.Pop();

			EvalStack.Push(EvaluateOperator(Item.GetType(), Arg1, Arg2, TempVariables, TempVariables));
		}
		else
		{
			EvalStack.Push(Item);
		}
	}

	// Must be one item left and must be an operand
	return EvalStack.Num() == 1 && EvalStack[0].IsOperand();
	
}

FSUDSValue FSUDSExpression::Evaluate(const TMap<FName, FSUDSValue>& Variables, const TMap<FName, FSUDSValue>& GlobalVariables) const
{
	checkf(bIsValid, TEXT("Cannot execute an invalid expression tree"));

	// Blanks are mostly used for conditionals, for simplicity always return true
	if (Queue.IsEmpty())
		return FSUDSValue(true);

	TArray<FSUDSExpressionItem> EvalStack;
	// We could pre-optimise all literal expressions, but let's not for now
	for (auto& Item : Queue)
	{
		if (Item.IsOperator())
		{
			FSUDSExpressionItem Arg1, Arg2;
			// Arg2 (RHS) has to be popped first
			if (Item.IsBinaryOperator())
			{
				checkf(!EvalStack.IsEmpty(), TEXT("Args missing before operator, bad expression"));
				Arg2 = EvalStack.Pop();
			}
			checkf(!EvalStack.IsEmpty(), TEXT("Args missing before operator, bad expression"));
			Arg1 = EvalStack.Pop();
			EvalStack.Push(EvaluateOperator(Item.GetType(), Arg1, Arg2, Variables, GlobalVariables));
		}
		else
		{
			EvalStack.Push(Item);
		}
	}
	
	checkf(EvalStack.Num() == 1, TEXT("We should end with a single item in the eval stack and it should be an operand"));

	return EvaluateOperand(EvalStack.Top().GetOperandValue(), Variables, GlobalVariables);
}

bool FSUDSExpression::EvaluateBoolean(const TMap<FName, FSUDSValue>& Variables, const TMap<FName, FSUDSValue>& GlobalVariables, const FString& ErrorContext) const
{
	const auto Result = Evaluate(Variables, GlobalVariables);

	if (Result.GetType() != ESUDSValueType::Boolean &&
		Result.GetType() != ESUDSValueType::Variable) // Allow unresolved variable, will assume false
	{
		UE_LOG(LogSUDS, Error, TEXT("%s: Condition '%s' did not return a boolean result"), *ErrorContext, *SourceString)
	}

	return Result.GetBooleanValue();
}

FSUDSExpressionItem FSUDSExpression::EvaluateOperator(ESUDSExpressionItemType Op,
                                                      const FSUDSExpressionItem& Arg1,
                                                      const FSUDSExpressionItem& Arg2,
                                                      const TMap<FName, FSUDSValue>& Variables,
                                                      const TMap<FName, FSUDSValue>& GlobalVariables) const
{
	const FSUDSValue Val1 = EvaluateOperand(Arg1.GetOperandValue(), Variables, GlobalVariables);
	FSUDSValue Val2;
	if (Arg1.IsBinaryOperator())
	{
		Val2 = EvaluateOperand(Arg2.GetOperandValue(), Variables, GlobalVariables);
	}

	switch (Op)
	{
	case ESUDSExpressionItemType::Not:
		return FSUDSExpressionItem(!Val1);
	case ESUDSExpressionItemType::Multiply:
		return FSUDSExpressionItem(Val1 * Val2);
	case ESUDSExpressionItemType::Divide:
		return FSUDSExpressionItem(Val1 / Val2);
	case ESUDSExpressionItemType::Add:
		return FSUDSExpressionItem(Val1 + Val2);
	case ESUDSExpressionItemType::Subtract:
		return FSUDSExpressionItem(Val1 - Val2);
	case ESUDSExpressionItemType::Less:
		return FSUDSExpressionItem(Val1 < Val2);
	case ESUDSExpressionItemType::LessEqual:
		return FSUDSExpressionItem(Val1 <= Val2);
	case ESUDSExpressionItemType::Greater:
		return FSUDSExpressionItem(Val1 > Val2);
	case ESUDSExpressionItemType::GreaterEqual:
		return FSUDSExpressionItem(Val1 >= Val2);
	case ESUDSExpressionItemType::Equal:
		return FSUDSExpressionItem(Val1 == Val2);
	case ESUDSExpressionItemType::NotEqual:
		return FSUDSExpressionItem(Val1 != Val2);
	case ESUDSExpressionItemType::And:
		return FSUDSExpressionItem(Val1 && Val2);
	case ESUDSExpressionItemType::Or:
		return FSUDSExpressionItem(Val1 || Val2);

		
	default: // these won't occur
	case ESUDSExpressionItemType::Null:
	case ESUDSExpressionItemType::Operand:
	case ESUDSExpressionItemType::LParens:
	case ESUDSExpressionItemType::RParens:
		return FSUDSExpressionItem();
	};
	
}

FSUDSValue FSUDSExpression::EvaluateOperand(const FSUDSValue& Operand,
                                            const TMap<FName, FSUDSValue>& Variables,
                                            const TMap<FName, FSUDSValue>& GlobalVariables) const
{
	// Simplify conversion to variable values
	if (Operand.IsVariable())
	{
		FName Name = Operand.GetVariableNameValue();
		FName GlobalName;
		if (USUDSLibrary::IsDialogueVariableGlobal(Name, GlobalName))
		{
			// This will have stripped the prefix so direct find is OK
			if (const auto Var = GlobalVariables.Find(GlobalName))
			{
				return *Var;
			}
		}
		if (const auto Var = Variables.Find(Operand.GetVariableNameValue()))
		{
			return *Var;
		}
		// Note: we're NOT warning about unset variables here, and just defaulting to initial values (false, 0 etc)
		// This is more usable in practice than complaining about it
	}

	return Operand;
}
