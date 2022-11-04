#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

void USUDSScriptNode::InitText(const FString& InSpeakerID, const FText& InText)
{
	NodeType = ESUDSScriptNodeType::Text;
	SpeakerID = InSpeakerID;
	Text = InText;
	TextFormat = Text;
	bFormatExtracted = false;
	
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

const FTextFormat& USUDSScriptNode::GetTextFormat() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return TextFormat;
}

const TArray<FString>& USUDSScriptNode::GetParameterNames() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return ParameterNames;
}

void USUDSScriptNode::ExtractFormat() const
{
	// Only do this on demand, and only once
	TextFormat = Text;
	ParameterNames.Empty();
	TextFormat.GetFormatArgumentNames(ParameterNames);
	bFormatExtracted = true;
}
