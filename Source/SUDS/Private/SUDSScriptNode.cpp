// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
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
	SourceLineNo = LineNo;
}

void USUDSScriptNode::InitReturn(int LineNo)
{
	NodeType = ESUDSScriptNodeType::Return;
	SourceLineNo = LineNo;
}

void USUDSScriptNode::AddEdge(const FSUDSScriptEdge& NewEdge)
{
	Edges.Add(NewEdge);
}

