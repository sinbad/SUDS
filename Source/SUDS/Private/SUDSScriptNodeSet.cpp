#include "SUDSScriptNodeSet.h"

void USUDSScriptNodeSet::Init(const FString& VarName, const FSUDSExpression& InExpression)
{
	NodeType = ESUDSScriptNodeType::SetVariable;
	Identifier = FName(VarName);
	Expression = InExpression;
}
