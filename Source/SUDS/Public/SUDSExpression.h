#pragma once
#include "SUDSValue.h"
#include "SUDSExpression.generated.h"

UENUM(BlueprintType)
enum class ESUDSExpressionItemType : uint8
{
	Null = 0 UMETA(Hidden),
	// Operators (must be 0-127, in order of precedence, highest first - gaps left in case we need them)
	Not = 4,
	Multiply = 10,
	Divide = 11,
	Add = 20,
	Subtract = 21,
	Less = 30,
	LessEqual = 31,
	Greater = 32,
	GreaterEqual = 33,
	Equal = 34,
	NotEqual = 35,
	And = 40,
	Or = 41,

	LParens = 100,
	RParens = 101,

	// Operands (must be 128+)
	Operand = 128
	
};

/// An item in an expression queue, can be operator or operand
USTRUCT(BlueprintType)
struct SUDS_API FSUDSExpressionItem
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	ESUDSExpressionItemType Type;

	// Value if an operand node
	UPROPERTY(BlueprintReadOnly)
	FSUDSValue OperandValue;

public:

	FSUDSExpressionItem() : Type(ESUDSExpressionItemType::Operand) {}
	FSUDSExpressionItem(ESUDSExpressionItemType Operator) : Type(Operator) {}

	FSUDSExpressionItem(const FSUDSValue& LiteralOrVariable)
		: Type(ESUDSExpressionItemType::Operand),
		  OperandValue(LiteralOrVariable)
	{
	}

	ESUDSExpressionItemType GetType() const { return Type; }
	// Only valid if optype is operand
	const FSUDSValue& GetOperandValue() const { return OperandValue; }
	void SetOperandValue(const FSUDSValue& NewVal) { OperandValue = NewVal; }

	bool IsOperator() const { return static_cast<uint8>(Type) < 128; }
	bool IsOperand() const { return !IsOperator(); }
	bool IsBinaryOperator() const
	{
		// Only not is unary right now
		return Type != ESUDSExpressionItemType::Not;
	}
};


/// An expression holds an executable expression, whether it's a simple single literal
/// or a compound expression with variables
USTRUCT(BlueprintType)
struct SUDS_API FSUDSExpression
{
	GENERATED_BODY()

protected:
	// The output queue in Reverse Polish Notation order
	UPROPERTY()
	TArray<FSUDSExpressionItem> Queue;

	/// Whether the tree is valid to execute
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid;

	/// The original string version of the expression, for reference 
	UPROPERTY(BlueprintReadOnly)
	FString SourceString;

	FSUDSExpressionItem EvaluateOperator(ESUDSExpressionItemType Op,
	                                       const FSUDSExpressionItem& Arg1,
	                                       const FSUDSExpressionItem& Arg2,
	                                       const TMap<FName, FSUDSValue>& Variables) const;
	FSUDSValue EvaluateOperand(const FSUDSValue& Operand, const TMap<FName, FSUDSValue>& Variables) const;

public:

	FSUDSExpression() : bIsValid(true) {}
	
	/// Initialise an expression tree just with a single literal or variable
	FSUDSExpression(const FSUDSValue& LiteralOrVariable)
	{
		Queue.Add(FSUDSExpressionItem(LiteralOrVariable));
		bIsValid = true;
	}

	/**
	 * Attempt to parse an expression from a string
	 * @param Expression The string to parse
	 * @param ErrorContext If there are any errors, what should the log prefix them with
	 * @return Whether the parsing was successful
	 */
	bool ParseFromString(const FString& Expression, const FString& ErrorContext);

	/// Evaluate the expression and return the result, using a given variable state 
	FSUDSValue Evaluate(const TMap<FName, FSUDSValue>& Variables) const;

	/// Evaluate the expression and return the result as a boolean, using a given variable state 
	bool EvaluateBoolean(const TMap<FName, FSUDSValue>& Variables, const FString& ErrorContext) const;

	/// Get the original source of the expression as a string
	const FString& GetSourceString() const { return SourceString; }

	/// Whether this expression can be run (or is empty)
	bool IsValid() const { return bIsValid; }

	/// Whether this expression is blank
	bool IsEmpty() const { return Queue.IsEmpty(); }


	/**
	 * Attempt to parse an operand from a string. Returns true if this string is a valid operand, which means a literal
	 * (int, float, quoted string, boolean, gender), or a variable reference ({VariableName})
	 * @param ValueStr The string to parse
	 * @param OutVal The operand value which will be populated if successful
	 * @return True if successful, false if not
	 */
	static bool ParseOperand(const FString& ValueStr, FSUDSValue& OutVal);
	
	// Attempt to parse an operator from an incoming string
	static ESUDSExpressionItemType ParseOperator(const FString& OpStr);

	/// Access the internal RPN execution queue
	const TArray<FSUDSExpressionItem>& GetQueue() { return Queue; }

	/// Return whether this is a single literal
	bool IsLiteral() const
	{
		return bIsValid && Queue.Num() == 1 && Queue[0].IsOperand() && Queue[0].GetOperandValue().GetType() != ESUDSValueType::Variable;
	}

	/// Helper method to get literal values
	FSUDSValue GetLiteralValue() const
	{
		check(IsLiteral());
		return Queue[0].GetOperandValue();
	}

	/// Return whenter this is a text literal
	bool IsTextLiteral() const
	{
		return bIsValid && Queue.Num() == 1 && Queue[0].IsOperand() && Queue[0].GetOperandValue().GetType() == ESUDSValueType::Text;
	}

	/// Helper method to get a text literal value, for easier localisation
	FText GetTextLiteralValue() const
	{
		check(IsTextLiteral());
		return GetLiteralValue().GetTextValue();
	}
	/// Helper method to override a text literal
	void SetTextLiteralValue(const FText& NewLiteral)
	{
		check(IsTextLiteral());
		Queue[0].SetOperandValue(NewLiteral);
	}

	/// Helper method to get boolean literal value
	bool GetBooleanLiteralValue() const
	{
		check(IsLiteral() && GetLiteralValue().GetType() == ESUDSValueType::Boolean);
		return GetLiteralValue().GetBooleanValue();
	}
	/// Helper method to get int literal value
	int GetIntLiteralValue() const
	{
		check(IsLiteral() && GetLiteralValue().GetType() == ESUDSValueType::Int);
		return GetLiteralValue().GetIntValue();
	}
	/// Helper method to get float literal value
	float GetFloatLiteralValue() const
	{
		check(IsLiteral() && GetLiteralValue().GetType() == ESUDSValueType::Float);
		return GetLiteralValue().GetFloatValue();
	}
	/// Helper method to get gender literal value
	ETextGender GetGenderLiteralValue() const
	{
		check(IsLiteral() && GetLiteralValue().GetType() == ESUDSValueType::Gender);
		return GetLiteralValue().GetGenderValue();
	}


};

