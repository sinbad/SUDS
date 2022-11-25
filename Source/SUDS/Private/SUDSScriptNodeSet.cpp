#include "SUDSScriptNodeSet.h"

void USUDSScriptNodeSet::Init(const FString& VarName, const FSUDSExpression& InExpression, int LineNo)
{
	NodeType = ESUDSScriptNodeType::SetVariable;
	Identifier = FName(VarName);
	Expression = InExpression;
	SourceLineNo = LineNo;
}
