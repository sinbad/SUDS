#include "SUDSScriptImporter.h"

#include "SUDSEditor.h"
#include "Internationalization/Regex.h"

PRAGMA_DISABLE_OPTIMIZATION

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
	IndentLevelStack.Empty();
	Nodes.Empty();
	JumpList.Empty();
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
	// Trim off any whitespace, but record how much of it there is since it can be relevant
	int IndentLevel;
	const FStringView TrimmedLine = TrimLine(Line, IndentLevel);

	if (TrimmedLine.Len() == 0 && !bTextInProgress)
	{
		// We will skip any blank lines that aren't inside text
		UE_LOG(LogSUDSEditor, VeryVerbose, TEXT("%3d:00: BLANK %s"), LineNo, *FString(Line));
		return true;
	}

	if (IsCommentLine(TrimmedLine))
	{
		// Skip over comment lines
		UE_LOG(LogSUDSEditor, VeryVerbose, TEXT("%3d:%2d: COMMENT %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}

	// Check for headers
	static const FStringView HeaderPrefix(TEXT("==="));
	if (TrimmedLine.StartsWith(HeaderPrefix))
	{
		if (bHeaderDone)
		{
			if (!bSilent)
			{
				UE_LOG(LogSUDSEditor, Error, TEXT("Failed to parse %s Line %d: Duplicate header section"), *NameForErrors, LineNo);
			}
			return false;
		}
		else if (bTooLateForHeader)
		{
			if (!bSilent)
			{
				UE_LOG(LogSUDSEditor, Error, TEXT("Failed to parse %s Line %d: Header section must be at start"), *NameForErrors, LineNo);
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
		return ParseHeaderLine(TrimmedLine, LineNo, NameForErrors, bSilent);
	}

	// Process body
	return ParseBodyLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, bSilent);
	
}

bool FSUDSScriptImporter::ParseHeaderLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent)
{
	// TODO parse header content

	UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:00: HEADER: %s"), LineNo, *FString(Line));
	return true;
}

bool FSUDSScriptImporter::ParseBodyLine(const FStringView& Line,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	bool bSilent)
{
	// Once we've had anything other than comments or blanks and non-headers, it's too late for headers
	bTooLateForHeader = true;
	
	// Body indenting matters
	// If less than "threshold indent", pop contexts from stack until that's no longer the case
	// If more than threshold indent, may be a child of previous line. Increase threshold indent to this level
	//   ONLY a new context level IF previous was condition or choice. If previous was text then just a continuation
	//   This is why "threshold indent" remains as the outermost indent in this context
	// If same as threshold indent, continuation of current context

	while (IndentLevelStack.Num() > 1 &&
		IndentLevel < IndentLevelStack.Top().ThresholdIndent)
	{
		// Pop as much from the stack as necessary to return to this indent level
		PopIndent();
	}

	if (IndentLevelStack.IsEmpty())
	{
		// Must be the first body line encountered. Add 1 indent level for the root
		PushIndent(0, 0);
	}

	if (Line.StartsWith(TEXT('*')))
	{
		return ParseChoiceLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	else if (Line.StartsWith(TEXT('[')))
	{
		bool bParsed = ParseConditionalLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseGotoLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseSetLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseEventLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);

		if (!bParsed)
		{
			UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: CMD  : %s"), LineNo, IndentLevel, *FString(Line));
			UE_LOG(LogSUDSEditor, Warning, TEXT("%s Line %d: Unrecognised command. Ignoring!"), *NameForErrors, LineNo);
			// We still return true because we don't want to fail the entire import
		}
		return true;
	}
	else
	{
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: TEXT  : %s"), LineNo, IndentLevel, *FString(Line));
		// Text line
	}
	


	return true;
	
	
}

bool FSUDSScriptImporter::ParseChoiceLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (Line.StartsWith('*'))
	{
		// TODO: Implement choice lines
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: CHOICE: %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseConditionalLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (Line.Equals(TEXT("[else]")))
	{
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: ELSE  : %s"), LineNo, IndentLevel, *FString(Line));
		// TODO create else condition edge
		return true;
		
	}
	else if (Line.Equals(TEXT("[endif]")))
	{
		// TODO finish conditional
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: ENDIF : %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	else
	{
		// Unfortunately FRegexMatcher doesn't support FStringView
		const FString LineStr(Line);
		const FRegexPattern IfPattern(TEXT("^\\[if\\s+(.+)\\]$"));
		FRegexMatcher IfRegex(IfPattern, LineStr);
		if (IfRegex.FindNext())
		{
			// TODO parse condition
			UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d:    IF : %s"), LineNo, IndentLevel, *FString(Line));
			return true;
		}
		else
		{
			const FRegexPattern ElseIfPattern(TEXT("^\\[elseif\\s+(.+)\\]$"));
			FRegexMatcher ElseIfRegex(ElseIfPattern, LineStr);
			if (ElseIfRegex.FindNext())
			{
				// TODO parse condition
				UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: ELSEIF: %s"), LineNo, IndentLevel, *FString(Line));
				return true;
			}
		}
	}
		
	return false;
}

bool FSUDSScriptImporter::ParseGotoLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	const FRegexPattern GotoPattern(TEXT("^\\[goto\\s+(\\w+)\\]$"));
	FRegexMatcher IfRegex(GotoPattern, LineStr);
	if (IfRegex.FindNext())
	{
		// TODO: Implement jump lines
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: GOTO  : %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseSetLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	const FRegexPattern SetPattern(TEXT("^\\[set\\s+(\\S+)\\s+(\\S+)\\]$"));
	FRegexMatcher IfRegex(SetPattern, LineStr);
	if (IfRegex.FindNext())
	{
		// TODO: Implement set lines
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: SET   : %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseEventLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	const FString LineStr(Line);
	const FRegexPattern EventPattern(TEXT("^\\[event\\s+(\\S+)\\s+(.+)\\]$"));
	FRegexMatcher IfRegex(EventPattern, LineStr);
	if (IfRegex.FindNext())
	{
		// TODO: Implement event lines
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: EVENT : %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	return false;
}

void FSUDSScriptImporter::PopIndent()
{
	IndentLevelStack.Pop();
}

void FSUDSScriptImporter::PushIndent(int NodeIdx, int Indent)
{
	IndentLevelStack.Add(IndentContext(NodeIdx, Indent));

}

bool FSUDSScriptImporter::IsCommentLine(const FStringView& TrimmedLine)
{
	return TrimmedLine.StartsWith('#');
}

FStringView FSUDSScriptImporter::TrimLine(const FStringView& Line, int& OutIndentLevel) const
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
	FStringView LeftTrimmed = FStringView(Line.GetData() + SkippedChars, Line.Len() - SkippedChars);
	// Trim end, don't need to know how much it was
	return LeftTrimmed.TrimEnd();
	
}

PRAGMA_ENABLE_OPTIMIZATION