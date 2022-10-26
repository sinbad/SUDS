#include "SUDSScriptImporter.h"

#include "SUDSEditor.h"
#include "Internationalization/Regex.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString FSUDSScriptImporter::DefaultJumpLabel = "::DEFAULT::";
const FString FSUDSScriptImporter::EndJumpLabel = "END";

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
	EdgeInProgress = nullptr;
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

	ConnectRemainingNodes(NameForErrors);

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
	// DeclaredSpeakers
	// Variables

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
		PushIndent(-1, 0);
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
		return ParseTextLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	


	return true;
	
	
}

bool FSUDSScriptImporter::ParseChoiceLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (Line.StartsWith('*'))
	{
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: CHOICE: %s"), LineNo, IndentLevel, *FString(Line));
		
		auto& Ctx = IndentLevelStack.Top();

		// If the current indent context node is NOT a choice, create a choice node, and connect to previous node (using pending edge if needed)
		if (!Nodes.IsValidIndex(Ctx.LastNodeIdx) ||
			Nodes[Ctx.LastNodeIdx].NodeType != ESUDSScriptNodeType::Choice)
		{
			// Last node was not a choice node, so to add edge for this choice we first need to create the choice node
			AppendNode(FSUDSParsedNode(ESUDSScriptNodeType::Choice, IndentLevel));
		}
		if (EdgeInProgress)
		{
			// Must already have been a choice node but previous pending edge wasn't resolved
			// This means it's a fallthrough, mark it as such
			MakeEdgeInProgressDefaultJump();
		}

		// Inside each choice, everything should be indented at least as much as 1 character inside the *
		PushIndent(Ctx.LastNodeIdx, IndentLevel + 1);
		
		// Add a pending edge, with the choice text
		// Following things fill in the edge details, the next node to be parsed will finalise the destination
		const FString ChoiceText(Line.SubStr(1, Line.Len() - 1).TrimStart());
		auto& ChoiceNode = Nodes[Ctx.LastNodeIdx];
		const int EdgeIdx = ChoiceNode.Edges.Add(FSUDSParsedEdge(-1, ChoiceText));
		EdgeInProgress = &ChoiceNode.Edges[EdgeIdx];
		
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
			// If creates a select node
			// Remember to connect up pending edges
			UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: IF    : %s"), LineNo, IndentLevel, *FString(Line));
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
	FRegexMatcher GotoRegex(GotoPattern, LineStr);
	if (GotoRegex.FindNext())
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
	FRegexMatcher EventRegex(SetPattern, LineStr);
	if (EventRegex.FindNext())
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
	FRegexMatcher EventRegex(EventPattern, LineStr);
	if (EventRegex.FindNext())
	{
		// TODO: Implement event lines
		UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: EVENT : %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseTextLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	auto& Ctx = IndentLevelStack.Top();

	const FString LineStr(Line);
	const FRegexPattern SpeakerPattern(TEXT("^(\\w+)\\:\\s*(.+)$"));
	FRegexMatcher SpeakerRegex(SpeakerPattern, LineStr);
	if (SpeakerRegex.FindNext())
	{
		// OK this might be a speaker line, in which case this is a new text node
		// However it might also be a new line in the existing speaker text node, if the word before the ":" is not
		// a recognised speaker
		// This is to allow cases where your text itself includes the words "Something: something else". You just need
		// to avoid having a speaker ID as the prefix
		// However, if you didn't define ANY speakers in the header, then we assume EVERY instance of "Something: Blah" is
		// a new speaker line. So you don't have to define speakers if you don't have this ambiguity or any other reason to.
		const FString Speaker = SpeakerRegex.GetCaptureGroup(1);
		const FString Text = SpeakerRegex.GetCaptureGroup(2);
		if (DeclaredSpeakers.Num() == 0 || DeclaredSpeakers.Contains(Speaker))
		{
			UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: TEXT  : %s"), LineNo, IndentLevel, *FString(Line));
			// New text node
			// Text nodes can never introduce another indent context
			// We've already backed out to the outer indent in caller
			AppendNode(FSUDSParsedNode(Speaker, Text, IndentLevel));
			
			return true;
		}
	}

	// If we fell through, this line is appended to the last text node
	UE_LOG(LogSUDSEditor, Verbose, TEXT("%3d:%2d: TEXT+ : %s"), LineNo, IndentLevel, *FString(Line));
	if (Nodes.IsValidIndex(Ctx.LastNodeIdx))
	{
		auto& Node = Nodes[Ctx.LastNodeIdx];
		if (Node.NodeType == ESUDSScriptNodeType::Text)
		{
			Node.Text.Appendf(TEXT("\n%s"), *LineStr);
		}
		else
		{
			UE_LOG(LogSUDSEditor, Warning, TEXT("Error in %s line %d: Text newline continuation is not immediately after a speaker line. Ignoring."), *NameForErrors, LineNo);
			// We still return true to allow continue	
		}
	}
	
	return true;


}

int FSUDSScriptImporter::AppendNode(const FSUDSParsedNode& NewNode)
{
	auto& Ctx = IndentLevelStack.Top();
	
	const int NewIndex = Nodes.Add(NewNode);
	if (Nodes.IsValidIndex(Ctx.LastNodeIdx))
	{
		// Append this node onto the last one
		auto& PrevNode = Nodes[Ctx.LastNodeIdx];
		// Use pending edge if present; that could be because this is under a choice node, or a condition
		if (EdgeInProgress)
		{
			EdgeInProgress->TargetNodeIdx = NewIndex;
		}
		else
		{
			// We only auto-connect new nodes to text nodes
			// Otherwise, we should be doing it via pending edges
			// E.g. choice nodes get edges created for choice options, select nodes for conditions
			// A new node with no pending edge following any other type may be connected via fallthrough at
			// the end of parsing
			if (PrevNode.NodeType == ESUDSScriptNodeType::Text)
			{
				PrevNode.Edges.Add(FSUDSParsedEdge(NewIndex));
			}
		}
		EdgeInProgress = nullptr;
	}

	Ctx.LastNodeIdx = NewIndex;
	Ctx.ThresholdIndent = FMath::Min(Ctx.ThresholdIndent, NewNode.OriginalIndent);
	

	return NewIndex;
}

void FSUDSScriptImporter::MakeEdgeInProgressDefaultJump()
{
	// Used to close off pending edges that don't get finished, will be checked to fall through later
	if (EdgeInProgress)
	{
		EdgeInProgress->bIsJump = true;
		EdgeInProgress->JumpTargetLabel = DefaultJumpLabel;
	}

	EdgeInProgress = nullptr;

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

void FSUDSScriptImporter::ConnectRemainingNodes(const FString& NameForErrors)
{
	// Now we go through all nodes, resolving jumps, and finding links that don't go anywhere * making
	// them fall through to the next appropriate outdented node (or the end)

	// We go through top-to-bottom, which is the order of lines in the file as well
	// We don't need to cascade for this
	for (int i = 0; i < Nodes.Num(); ++i)
	{
		auto& Node = Nodes[i];
		if (Node.Edges.IsEmpty())
		{
			// Find the next node which is at a higher indent level than this
			const auto FallthroughIdx = FindNextOutdentedNodeIndex(i+1, Node.OriginalIndent);
			if (Nodes.IsValidIndex(FallthroughIdx))
			{
				Node.Edges.Add(FSUDSParsedEdge(FallthroughIdx));
			}
			else
			{
				// If no node to fallthrough to, jump to end
				Node.Edges.Add(FSUDSParsedEdge(EndJumpLabel));
			}
		}
		else
		{
			for (auto& Edge : Node.Edges)
			{
				if (Edge.bIsJump)
				{
					if (Edge.JumpTargetLabel == DefaultJumpLabel)
					{
						// This is a fallthrough jump
						// Usually this is a choice line without anything under it, or a condition with nothing in it
						const auto FallthroughIdx = FindNextOutdentedNodeIndex(i+1, Node.OriginalIndent);
						if (Nodes.IsValidIndex(FallthroughIdx))
						{
							Edge.TargetNodeIdx = FallthroughIdx;
						}
						else
						{
							// If no node to fallthrough to, jump to end
							Edge.TargetNodeIdx = -1;
							Edge.JumpTargetLabel = EndJumpLabel;
						}
					}
					else if (!Nodes.IsValidIndex(Edge.TargetNodeIdx))
					{
						// Resolve using jump list
						int *pJumpIdx = JumpList.Find(Edge.JumpTargetLabel);
						if (pJumpIdx)
						{
							Edge.TargetNodeIdx = *pJumpIdx;
						}
						else
						{
							UE_LOG(LogSUDSEditor, Warning, TEXT("Error in %s: Goto label '%s' was not found, references to it will goto END"), *NameForErrors, *Edge.JumpTargetLabel)
						}
						
					}
				}
				
			}
		}
	}
}

int FSUDSScriptImporter::FindNextOutdentedNodeIndex(int StartNodeIndex, int IndentLessThan)
{
	for (int i = StartNodeIndex; i < Nodes.Num(); ++i)
	{
		auto N = Nodes[i];
		if (N.OriginalIndent < IndentLessThan)
		{
			return i;
		}
		
	}

	return -1;
}

const FSUDSParsedNode* FSUDSScriptImporter::GetNode(int Index)
{
	if (Nodes.IsValidIndex(Index))
	{
		return &Nodes[Index];
	}

	return nullptr;
}

PRAGMA_ENABLE_OPTIMIZATION
