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
	FSUDSValue GetOperandValue() const { return OperandValue; }

	bool IsOperator() const { return (static_cast<uint8>(Type) & 0x0F) > 0; }
	bool IsOperand() const { return !IsOperator(); }
};


/// An expression holds an executable expression, whether it's a simple single literal
/// or a compound expression with variables
USTRUCT(BlueprintType)
struct SUDS_API FSUDSExpression
{
	GENERATED_BODY()

protected:
	// The output queue in Reverse Polish Notation order
	TArray<FSUDSExpressionItem> Queue;

	/// Whether the tree is valid to execute
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid;

public:

	FSUDSExpression() : bIsValid(false) {}
	
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

	/// Execute the expression and return the result, using a given variable state 
	FSUDSValue Execute(const TMap<FName, FSUDSValue>& Variables) const;


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

};

