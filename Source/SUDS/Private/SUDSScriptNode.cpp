#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitText(const FString& InSpeakerID, const FString& InTextID)
{
	NodeType = ESUDSScriptNodeType::Text;
	SpeakerID = InSpeakerID;
	TextID = InTextID;
	// TODO: localisation will remove this
	TempText = FText::FromString(InTextID);
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
