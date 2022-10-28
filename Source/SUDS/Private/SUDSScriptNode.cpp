// Copyright 2020 Old Doorways Ltd


#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitText(const FString& InSpeaker, const FString& InText)
{
	NodeType = ESUDSScriptNodeType::Text;
	Speaker = InSpeaker;
	TempText = InText;
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
