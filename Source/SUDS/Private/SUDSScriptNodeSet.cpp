#include "SUDSScriptNodeSet.h"

void USUDSScriptNodeSet::Init(const FString& VarName, const FSUDSValue& LiteralValue)
{
	NodeType = ESUDSScriptNodeType::SetVariable;
	Identifier = FName(VarName);
	Literal = LiteralValue;
}
