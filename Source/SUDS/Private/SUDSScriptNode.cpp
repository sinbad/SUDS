#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitText(const FString& InSpeakerID, const FText& InText)
{
	NodeType = ESUDSScriptNodeType::Text;
	SpeakerID = InSpeakerID;
	Text = InText;
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
