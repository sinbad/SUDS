#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitChoice()
{
	NodeType = ESUDSScriptNodeType::Choice;
}

void USUDSScriptNode::InitSelect()
{
	NodeType = ESUDSScriptNodeType::Select;
}

void USUDSScriptNode::AddEdge(const FSUDSScriptEdge& NewEdge)
{
	Edges.Add(NewEdge);
}

