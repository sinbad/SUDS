#include "SUDSScriptImporter.h"

#include "SUDSEditor.h"

bool FSUDSScriptImporter::ImportFromBuffer(const TCHAR *Start, int32 Length, const FString& NameForErrors, bool bSilent)
{
	static const TCHAR* LineEndings[] =
	{				
		TEXT("\r\n"),
		TEXT("\r"),
		TEXT("\n"),	
	};
	constexpr int32 NumDelims = UE_ARRAY_COUNT(LineEndings);

	int LineNumber = 1;
	bHeaderDone = false;
	bHeaderInProgress = false;
	bool bImportedOK = true;
	if (Start)
	{
		int32 SubstringBeginIndex = 0;

		// Iterate through string.
		for(int32 i = 0; i < Length;)
		{
			int32 SubstringEndIndex = INDEX_NONE;
			int32 DelimiterLength = 0;

			// Attempt each delimiter.
			for(int32 DelimIndex = 0; DelimIndex < NumDelims; ++DelimIndex)
			{
				DelimiterLength = FCString::Strlen(LineEndings[DelimIndex]);

				// If we found a delimiter...
				if (FCString::Strncmp(Start + i, LineEndings[DelimIndex], DelimiterLength) == 0)
				{
					// Mark the end of the substring.
					SubstringEndIndex = i;
					break;
				}
			}

			if (SubstringEndIndex != INDEX_NONE)
			{
				const int32 SubstringLength = SubstringEndIndex - SubstringBeginIndex;
				FStringView Line = MakeStringView(Start + SubstringBeginIndex, SubstringLength);
				if (!ParseLine(Line, LineNumber++, NameForErrors, bSilent))
				{
					// Abort, error
					bImportedOK = false;
					break;
				}
				
				
				// Next substring begins at the end of the discovered delimiter.
				SubstringBeginIndex = SubstringEndIndex + DelimiterLength;
				i = SubstringBeginIndex;
			}
			else
			{
				++i;
			}
		}

		// Add any remaining characters after the last delimiter.
		const int32 SubstringLength = Length - SubstringBeginIndex;
		FStringView Line = MakeStringView(Start + SubstringBeginIndex, SubstringLength);
		bImportedOK = ParseLine(Line, LineNumber++, NameForErrors, bSilent);
	}

	return bImportedOK;
	
}

bool FSUDSScriptImporter::ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (Line.Len() == 0 && !bTextInProgress)
	{
		// We will skip any blank lines that aren't inside text
		return true;
	}

	// Check for headers
	static const FStringView HeaderPrefix(TEXT("==="));
	if (Line.StartsWith(HeaderPrefix))
	{
		if (bHeaderDone)
		{
			if (!bSilent)
			{
				UE_LOG(LogSUDSEditor, Warning, TEXT("Failed to parse %s Line %d: Duplicate header section"), *NameForErrors, LineNo);
			}
			return false;
		}
		
		if (bHeaderInProgress)
		{
			// End of header
			bHeaderInProgress = false;
			bHeaderDone = true;
		}
		else
		{
			bHeaderInProgress = true;
		}
		return true;
	}
	else if (bHeaderInProgress)
	{
		return ParseHeaderLine(Line, LineNo, NameForErrors, bSilent);
	}

	// Process body

	UE_LOG(LogSUDSEditor, Log, TEXT("%d: BODY  : %s"), LineNo, *FString(Line));
	
	// Trim off any leading whitespace, but record how much of it there is since it's relevant
	// If less than current indent, pop context from stack
	// If more than current indent, then a child of previous line
	// If same as current indent, continuation of current context
	int IndentLevel;
	FStringView Trimmed = TrimLine(Line, &IndentLevel);


	return true;
	
}

bool FSUDSScriptImporter::ParseHeaderLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent)
{
	// TODO parse header content

	UE_LOG(LogSUDSEditor, Log, TEXT("%d: HEADER: %s"), LineNo, *FString(Line));
	return true;
}

FStringView FSUDSScriptImporter::TrimLine(const FStringView& Line, int* OutIndentLevel) const
{
	OutIndentLevel = 0;
	int32 SkippedChars = 0;
	for (const TCHAR Char : Line)
	{
		if (!TChar<TCHAR>::IsWhitespace(Char))
		{
			break;
		}
		++SkippedChars;
		if (Char == '\t')
			OutIndentLevel += TabIndentValue;
		else
			++OutIndentLevel;
	}
	return FStringView(Line.GetData() + SkippedChars, Line.Len() - SkippedChars);
	
}
