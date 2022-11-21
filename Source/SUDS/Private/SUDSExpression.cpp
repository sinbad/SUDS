#include "SUDSExpression.h"

#include "Misc/DefaultValueHelper.h"

bool FSUDSExpression::ParseFromString(const FString& Expression, const FString& ErrorContext)
{
	// Assume invalid until we've parsed something
	bIsValid = false;
	Queue.Empty();
	
	// Shunting-yard algorithm
	// Thanks to Nathan Reed https://www.reedbeta.com/blog/the-shunting-yard-algorithm/
	// We take a natural string, parse it and then turn it into a queue of tokens (operators and operands)
	// expressed in Reverse Polish Notation, which can be easily executed later
	// Variables are not resolved at this point, only at execution time.
	
	// Split into individual tokens; sections of the regex are:
	// - {Variable}
	// - Arithmetic operators & parentheses
	// - Literal numbers (with or without decimal point)
	// - Boolean operators & comparisions
	// - Quoted strings (group 1 includes quotes, group 2 is trimmed)
	const FRegexPattern Pattern(TEXT("(\\{\\w+}|[-+*/\\(\\)]|\\d+(?:\\.\\d*)?|and|&&|\\|\\||or|not|\\<\\>|!=|\\<=?|\\>=?|==?)|\\\"([^\\\"]*)\\\""));
	FRegexMatcher Regex(Pattern, Expression);
	// Stacks that we use to construct
	TArray<ESUDSExpressionNodeType> OperatorStack;
	bool bParsedSomething = false;
	bool bErrors = false;
	while (Regex.FindNext())
	{
		FString Str = Regex.GetCaptureGroup(1);
		ESUDSExpressionNodeType OpType = ParseOperator(Str);
		if (OpType != ESUDSExpressionNodeType::Null)
		{
			bParsedSomething = true;

			if (OpType == ESUDSExpressionNodeType::LParens)
			{
				OperatorStack.Push(OpType);
			}
			else if (OpType == ESUDSExpressionNodeType::RParens)
			{
				if (OperatorStack.IsEmpty())
				{
					UE_LOG(LogSUDS, Error, TEXT("Error in %s: mismatched parentheses"), *ErrorContext);
					bErrors = true;
					break;
				}
					
				while (OperatorStack.Num() > 0 && OperatorStack.Top() != ESUDSExpressionNodeType::LParens)
				{
					Queue.Add(FSUDSExpressionItem(OperatorStack.Pop()));
				}
				if (OperatorStack.IsEmpty())
				{
					UE_LOG(LogSUDS, Error, TEXT("Error in %s: mismatched parentheses"), *ErrorContext);
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
				const bool bLeftAssociative = OpType != ESUDSExpressionNodeType::Not;
				// Valid operator
				// Apply anything on the operator stack which is higher / equal precedence
				while (OperatorStack.Num() > 0 &&
					// higher precedence applied now, and equal precedence if left-associative
					(static_cast<int>(OperatorStack.Top()) < static_cast<int>(OpType) ||
					static_cast<int>(OperatorStack.Top()) <= static_cast<int>(OpType)	&& bLeftAssociative))
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
				UE_LOG(LogSUDS, Error, TEXT("Error in %s: unrecognised token %s"), *ErrorContext, *Str);
				bErrors = true;
			}
		}
	}
	// finish up
	while (OperatorStack.Num() > 0)
	{
		if (OperatorStack.Top() == ESUDSExpressionNodeType::LParens ||
			OperatorStack.Top() == ESUDSExpressionNodeType::RParens)
		{
			bErrors = true;
			UE_LOG(LogSUDS, Error, TEXT("Error in %s: mismatched parentheses"), *ErrorContext);
			break;
		}

		Queue.Add(FSUDSExpressionItem(OperatorStack.Pop()));
	}

	bIsValid = bParsedSomething && !bErrors;

	return bIsValid;
}


ESUDSExpressionNodeType FSUDSExpression::ParseOperator(const FString& OpStr)
{
	return ESUDSExpressionNodeType::Null;
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
		const FRegexPattern Pattern(TEXT("^\\\"([^\\\"]*)\\\"$"));
		FRegexMatcher Regex(Pattern, ValueStr);
		if (Regex.FindNext())
		{
			const FString Val = Regex.GetCaptureGroup(1);
			OutVal = FSUDSValue(FText::FromString(Val));
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
			OutVal = FSUDSValue(VariableName);
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


FSUDSValue FSUDSExpression::Execute(const TMap<FName, FSUDSValue>& Variables) const
{
	checkf(bIsValid, TEXT("Cannot execute an invalid expression tree"));

	// Short-circuit simplest case
	if (Queue.Num() == 1 && Queue[0].GetType() == ESUDSExpressionNodeType::Operand)
	{
		return Queue[0].GetOperandValue();
	}

	// Placeholder
	return FSUDSValue();
}
