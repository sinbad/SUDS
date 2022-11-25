#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitChoice(int LineNo)
{
	NodeType = ESUDSScriptNodeType::Choice;
	SourceLineNo = LineNo;
}

void USUDSScriptNode::InitSelect(int LineNo)
{
	NodeType = ESUDSScriptNodeType::Select;
}

void USUDSScriptNode::AddEdge(const FSUDSScriptEdge& NewEdge)
{
	Edges.Add(NewEdge);
}

