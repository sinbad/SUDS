#include "SUDSScriptNodeText.h"

void USUDSScriptNodeText::Init(const FString& InSpeakerID, const FText& InText)
{
	NodeType = ESUDSScriptNodeType::Text;
	SpeakerID = InSpeakerID;
	Text = InText;
	TextFormat = Text;
	bFormatExtracted = false;
	
}

const FTextFormat& USUDSScriptNodeText::GetTextFormat() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return TextFormat;
}

const TArray<FString>& USUDSScriptNodeText::GetParameterNames() const
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
	TextFormat.GetFormatArgumentNames(ParameterNames);
	bFormatExtracted = true;
}
