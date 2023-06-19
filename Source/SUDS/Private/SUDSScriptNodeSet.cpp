// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptNodeSet.h"

void USUDSScriptNodeSet::Init(const FString& VarName, const FSUDSExpression& InExpression, int LineNo)
{
	NodeType = ESUDSScriptNodeType::SetVariable;
	Identifier = FName(VarName);
	Expression = InExpression;
	SourceLineNo = LineNo;
}
