#include "SUDSScriptEdge.h"

void FSUDSScriptEdge::ExtractFormat() const
{
	// Only do this on demand, and only once
	TextFormat = Text;
	ParameterNames.Empty();
	TextFormat.GetFormatArgumentNames(ParameterNames);
	bFormatExtracted = true;
}

void FSUDSScriptEdge::SetText(const FText& InText)
{
	Text = InText;
	bFormatExtracted = false;
}

const FTextFormat& FSUDSScriptEdge::GetTextFormat() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return TextFormat;
	
}

const TArray<FString>& FSUDSScriptEdge::GetParameterNames() const
{
	if (!bFormatExtracted)
	{
		ExtractFormat();
	}
	return ParameterNames;
	
}
