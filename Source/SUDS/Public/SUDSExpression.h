#pragma once
#include "SUDSValue.h"
#include "SUDSExpression.generated.h"

UENUM(BlueprintType)
enum class ESUDSExpressionNodeType : uint8
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

	// Operands (must be 128+)
	Operand = 128
	
};

/// A node in an expression tree
USTRUCT(BlueprintType)
struct FSUDSExpressionNode
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	ESUDSExpressionNodeType Type;

	UPROPERTY(BlueprintReadOnly)
	FSUDSValue OperandValue;
	
	UPROPERTY(BlueprintReadOnly)
	int LhsIdx = -1;
	UPROPERTY(BlueprintReadOnly)
	int RhsIdx = -1;

public:

	FSUDSExpressionNode() : Type(ESUDSExpressionNodeType::Operand) {}

	FSUDSExpressionNode(const FSUDSValue& LiteralOrVariable)
		: Type(ESUDSExpressionNodeType::Operand),
		  OperandValue(LiteralOrVariable)
	{
	}

	ESUDSExpressionNodeType GetType() const { return Type; }
	FSUDSValue GetOperandValue() const { return OperandValue; }
	int GetLhsIdx() const { return LhsIdx; }
	int GetRhsIdx() const { return RhsIdx; }

	bool IsOperator() const { return (static_cast<uint8>(Type) & 0x0F) > 0; }
	bool IsOperand() const { return !IsOperator(); }
};


/// An expression tree can hold an executable expression, whether it's a simple single literal
/// or a compound expression with variables
USTRUCT(BlueprintType)
struct FSUDSExpressionTree
{
	GENERATED_BODY()

protected:
	TArray<FSUDSExpressionNode> Tree;

	/// Whether the tree is valid to execute
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid;

public:

	FSUDSExpressionTree() : bIsValid(false) {}
	
	/// Initialise an expression tree just with a single literal or variable
	FSUDSExpressionTree(const FSUDSValue& LiteralOrVariable)
	{
		Tree.Add(FSUDSExpressionNode(LiteralOrVariable));
		bIsValid = true;
	}
	/// Construct from expression string
	FSUDSExpressionTree(const FStringView& Expression)
	{
		ParseFromString(Expression);
	}
	/// Set tree from expression string
	void ParseFromString(const FStringView& Expression);

	/// Execute the tree and return the result, using a given variable state 
	FSUDSValue Execute(const TMap<FName, FSUDSValue>& Variables) const;
	
};

