#include "SUDSScriptNodeSet.h"

void USUDSScriptNodeSet::Init(const FString& VarName, const FFormatArgumentValue& LiteralValue)
{
	NodeType = ESUDSScriptNodeType::SetVariable;
	Identifier = VarName;
	Literal = LiteralValue;
}
