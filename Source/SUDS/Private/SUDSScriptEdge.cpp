// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptEdge.h"

#include "SUDSScriptNode.h"


FSUDSScriptEdge::FSUDSScriptEdge(USUDSScriptNode* ToNode, ESUDSEdgeType InType, int LineNo): Type(InType),
	TargetNode(ToNode),
	SourceLineNo(LineNo)
{
}

FSUDSScriptEdge::FSUDSScriptEdge(const FText& InText, USUDSScriptNode* ToNode, int LineNo): Text(InText),
	Type(ESUDSEdgeType::Decision),
	TargetNode(ToNode),
	SourceLineNo(LineNo)
{
}

void FSUDSScriptEdge::ExtractFormat() const
{
	// Only do this on demand, and only once
	TextFormat = Text;
	ParameterNames.Empty();
	TArray<FString> TextParams;
	TextFormat.GetFormatArgumentNames(TextParams);
	for (auto Param : TextParams)
	{
		ParameterNames.Add(FName(Param));
	}
	bFormatExtracted = true;
}

FString FSUDSScriptEdge::GetTextID() const
{
	return FTextInspector::GetTextId(Text).GetKey().GetChars();
}

void FSUDSScriptEdge::SetText(const FText& InText)
{
	Text = InText;
	bFormatExtracted = false;
}

void FSUDSScriptEdge::SetTargetNode(const TWeakObjectPtr<USUDSScriptNode>& InTargetNode)
{
	TargetNode = InTargetNode;
}

const FTextFormat& FSUDSScriptEdge::GetTextFormat() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return TextFormat;
	
}

const TArray<FName>& FSUDSScriptEdge::GetParameterNames() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return ParameterNames;
	
}

bool FSUDSScriptEdge::HasParameters() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return !ParameterNames.IsEmpty();
	
}
