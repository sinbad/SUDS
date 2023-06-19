// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptNodeText.h"

void USUDSScriptNodeText::Init(const FString& InSpeakerID, const FText& InText, int LineNo)
{
	NodeType = ESUDSScriptNodeType::Text;
	SpeakerID = InSpeakerID;
	Text = InText;
	TextFormat = Text;
	SourceLineNo = LineNo;
	bFormatExtracted = false;
	
}

FString USUDSScriptNodeText::GetTextID() const
{
	return FTextInspector::GetTextId(Text).GetKey().GetChars();
}

const FTextFormat& USUDSScriptNodeText::GetTextFormat() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return TextFormat;
}

const TArray<FName>& USUDSScriptNodeText::GetParameterNames() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return ParameterNames;
}

bool USUDSScriptNodeText::HasParameters() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return !ParameterNames.IsEmpty();
	
}

void USUDSScriptNodeText::ExtractFormat() const
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
