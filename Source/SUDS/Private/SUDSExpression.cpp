#include "SUDSExpression.h"

void FSUDSExpressionTree::ParseFromString(const FStringView& Expression)
{
	// Assume invalid until we've parsed something
	bIsValid = false;
	// Shunting-yard algorithm
	// Thanks to Nathan Reed https://www.reedbeta.com/blog/the-shunting-yard-algorithm/

	


	
}

FSUDSValue FSUDSExpressionTree::Execute(const TMap<FName, FSUDSValue>& Variables) const
{
	checkf(bIsValid, TEXT("Cannot execute an invalid expression tree"));

	// Short-circuit simplest case
	if (Tree.Num() == 1 && Tree[0].GetType() == ESUDSExpressionNodeType::Operand)
	{
		return Tree[0].GetOperandValue();
	}

	// Placeholder
	return FSUDSValue();
}
