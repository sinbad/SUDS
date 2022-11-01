#include "SUDSScriptImporter.h"

#include "SUDSScript.h"
#include "SUDSScriptNode.h"
#include "Internationalization/Regex.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString FSUDSScriptImporter::EndGotoLabel = "end";
const FString FSUDSScriptImporter::TreePathSeparator = "/";

DEFINE_LOG_CATEGORY(LogSUDSImporter)

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
	PendingGotoLabels.Empty();
	AliasedGotoLabels.Empty();
	bHeaderDone = false;
	bHeaderInProgress = false;
	bool bImportedOK = true;
	IndentLevelStack.Empty();
	Nodes.Empty();
	GotoLabelList.Empty();
	ChoiceUniqueId = 0;
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

	ConnectRemainingNodes(NameForErrors, bSilent);

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
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:00: BLANK %s"), LineNo, *FString(Line));
		return true;
	}

	if (IsCommentLine(TrimmedLine))
	{
		// Skip over comment lines
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: COMMENT %s"), LineNo, IndentLevel, *FString(Line));
		return true;
	}

	// Check for headers
	static const FStringView HeaderPrefix(TEXT("==="));
	if (TrimmedLine.StartsWith(HeaderPrefix))
	{
		if (bHeaderDone)
		{
			if (!bSilent)
				UE_LOG(LogSUDSImporter, Error, TEXT("Failed to parse %s Line %d: Duplicate header section"), *NameForErrors, LineNo);
			return false;
		}
		else if (bTooLateForHeader)
		{
			if (!bSilent)
				UE_LOG(LogSUDSImporter, Error, TEXT("Failed to parse %s Line %d: Header section must be at start"), *NameForErrors, LineNo);
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

	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:00: HEADER: %s"), LineNo, *FString(Line));
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
		PushIndent(-1, 0, "");
	}

	if (Line.StartsWith(TEXT('*')))
	{
		return ParseChoiceLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	else if (Line.StartsWith(TEXT(':')))
	{
		return ParseGotoLabelLine(Line, IndentLevel, LineNo, NameForErrors, bSilent);
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
			if (!bSilent)
			{
				UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: CMD  : %s"), LineNo, IndentLevel, *FString(Line));
				UE_LOG(LogSUDSImporter, Warning, TEXT("%s Line %d: Unrecognised command. Ignoring!"), *NameForErrors, LineNo);
			}
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
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: CHOICE: %s"), LineNo, IndentLevel, *FString(Line));
		
		auto& Ctx = IndentLevelStack.Top();

		// If the current indent context node is NOT a choice, create a choice node, and connect to previous node (using pending edge if needed)
		if (!Nodes.IsValidIndex(Ctx.LastNodeIdx) ||
			Nodes[Ctx.LastNodeIdx].NodeType != ESUDSParsedNodeType::Choice)
		{
			// Last node was not a choice node, so to add edge for this choice we first need to create the choice node
			AppendNode(FSUDSParsedNode(ESUDSParsedNodeType::Choice, IndentLevel));
		}
		if (EdgeInProgress)
		{
			// Must already have been a choice node but previous pending edge wasn't resolved
			// This means it's a fallthrough, will be resolved at end
			EdgeInProgress = nullptr;			
		}

		auto& ChoiceNode = Nodes[Ctx.LastNodeIdx];
		
		// Inside each choice, everything should be indented at least as much as 1 character inside the *
		// We provide the edge with context C001, C002 etc for fallthrough
		PushIndent(Ctx.LastNodeIdx, IndentLevel + 1, FString::Printf(TEXT("C%03d"), ++ChoiceUniqueId));
		
		// Add a pending edge, with the choice text
		// Following things fill in the edge details, the next node to be parsed will finalise the destination
		const FString ChoiceText(Line.SubStr(1, Line.Len() - 1).TrimStart());
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
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ELSE  : %s"), LineNo, IndentLevel, *FString(Line));
		// TODO create else condition edge
		return true;
		
	}
	else if (Line.Equals(TEXT("[endif]")))
	{
		// TODO finish conditional
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ENDIF : %s"), LineNo, IndentLevel, *FString(Line));
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
			if (!bSilent)
				UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: IF    : %s"), LineNo, IndentLevel, *FString(Line));
			return true;
		}
		else
		{
			const FRegexPattern ElseIfPattern(TEXT("^\\[elseif\\s+(.+)\\]$"));
			FRegexMatcher ElseIfRegex(ElseIfPattern, LineStr);
			if (ElseIfRegex.FindNext())
			{
				// TODO parse condition
				if (!bSilent)
					UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ELSEIF: %s"), LineNo, IndentLevel, *FString(Line));
				return true;
			}
		}
	}
		
	return false;
}

bool FSUDSScriptImporter::ParseGotoLabelLine(const FStringView& Line, int IndentLevel, int LineNo,	const FString& NameForErrors, bool bSilent)
{
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: LABEL : %s"), LineNo, IndentLevel, *FString(Line));
	// We've already established that line starts with ':'
	// There should not be any spaces in the label
	const FString LineStr(Line);
	const FRegexPattern LabelPattern(TEXT("^\\:\\s*(\\w+)$"));
	FRegexMatcher LabelRegex(LabelPattern, LineStr);
	if (LabelRegex.FindNext())
	{
		// lowercase goto labels so case insensitive
		FString Label = LabelRegex.GetCaptureGroup(1).ToLower();
		if (Label == EndGotoLabel)
		{
			if (!bSilent)
				UE_LOG(LogSUDSImporter, Error, TEXT("Error in %s line %d: Label 'end' is reserved and cannot be used in the script, ignoring"), *NameForErrors, LineNo);
		}
		else
		{
			PendingGotoLabels.Add(Label);
			// This will be connected to the next node created
			// Is a list since multiple labels may resolve to the same place
		}
	}
	else
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, Warning, TEXT("Error in %s line %d: Badly formed goto label"), *NameForErrors, LineNo);
	}
	// Always return true to carry on, may not be used
	return true;
}

bool FSUDSScriptImporter::ParseGotoLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: GOTO  : %s"), LineNo, IndentLevel, *FString(Line));
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	// Allow both 'goto' and 'go to'
	const FRegexPattern GotoPattern(TEXT("^\\[go[ ]?to\\s+(\\w+)\\s*\\]$"));
	FRegexMatcher GotoRegex(GotoPattern, LineStr);
	if (GotoRegex.FindNext())
	{
		// lower case label so case insensitive
		const FString Label = GotoRegex.GetCaptureGroup(1).ToLower();
		// note that we do NOT try to resolve the goto label here, to allow forward jumps.
		const auto& Ctx = IndentLevelStack.Top();
		// A goto is an edge from the current node to another node
		// That means if we had pending labels that didn't hit a node before now, they're just aliases to THIS label
		for (auto PendingLabel : PendingGotoLabels)
		{
			AliasedGotoLabels.Add(PendingLabel, Label);
		}
		PendingGotoLabels.Reset();
		AppendNode(FSUDSParsedNode(Label, IndentLevel));
		return true;
	}
	if (!bSilent)
		UE_LOG(LogSUDSImporter, Error, TEXT("Error in %s line %d: malformed goto line"), *NameForErrors, LineNo);
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
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: SET   : %s"), LineNo, IndentLevel, *FString(Line));
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
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: EVENT : %s"), LineNo, IndentLevel, *FString(Line));
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
			if (!bSilent)
				UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: TEXT  : %s"), LineNo, IndentLevel, *FString(Line));
			// New text node
			// Text nodes can never introduce another indent context
			// We've already backed out to the outer indent in caller
			AppendNode(FSUDSParsedNode(Speaker, Text, IndentLevel));
			
			return true;
		}
	}

	// If we fell through, this line is appended to the last text node
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: TEXT+ : %s"), LineNo, IndentLevel, *FString(Line));
	if (Nodes.IsValidIndex(Ctx.LastNodeIdx))
	{
		auto& Node = Nodes[Ctx.LastNodeIdx];
		if (Node.NodeType == ESUDSParsedNodeType::Text)
		{
			Node.Text.Appendf(TEXT("\n%s"), *LineStr);
		}
		else
		{
			if (!bSilent)
				UE_LOG(LogSUDSImporter, Warning, TEXT("Error in %s line %d: Text newline continuation is not immediately after a speaker line. Ignoring."), *NameForErrors, LineNo);
			// We still return true to allow continue	
		}
	}
	
	return true;


}

FString FSUDSScriptImporter::GetCurrentTreePath()
{
	// This is just a path of all the choice / select nodes AND their edges leading to this point, for fallthrough
	// * Choice (/C000/)
	//		* Nested choice (/C000/C001/)
	//			Fallthrough from here
	// * Choice (/C002/C003/)
	//		Do NOT fallthrough to here
	// Fallthrough to here instead (/)

	FStringBuilderBase B;
	for (auto Indent : IndentLevelStack)
	{
		B.Appendf(TEXT("%s%s"), *Indent.PathEntry, *TreePathSeparator);
	}
	return B.ToString();
}

int FSUDSScriptImporter::AppendNode(const FSUDSParsedNode& NewNode)
{
	auto& Ctx = IndentLevelStack.Top();

	const int NewIndex = Nodes.Add(NewNode);

	// Set the tree path of the node (post-add)
	{
		auto& N = Nodes[NewIndex];
		N.TreePath = GetCurrentTreePath();
	}
	
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
			if (PrevNode.NodeType == ESUDSParsedNodeType::Text)
			{
				PrevNode.Edges.Add(FSUDSParsedEdge(NewIndex));
			}
		}
		EdgeInProgress = nullptr;
	}

	// All goto labels in scope at this point now point to this node
	for (auto Label : PendingGotoLabels)
	{
		GotoLabelList.Add(Label, NewIndex);
	}
	PendingGotoLabels.Reset();

	Ctx.LastNodeIdx = NewIndex;
	Ctx.ThresholdIndent = FMath::Min(Ctx.ThresholdIndent, NewNode.OriginalIndent);
	

	return NewIndex;
}

void FSUDSScriptImporter::PopIndent()
{
	IndentLevelStack.Pop();
	// We *could* reset the pending goto list if there are labels at higher indents than current that never resolved to a node
	// but I'm choosing not to right now and letting them fall through
}

void FSUDSScriptImporter::PushIndent(int NodeIdx, int Indent, const FString& Path)
{
	IndentLevelStack.Push(IndentContext(NodeIdx, Indent, Path));

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

void FSUDSScriptImporter::ConnectRemainingNodes(const FString& NameForErrors, bool bSilent)
{
	// Now we go through all nodes, resolving gotos, and finding links that don't go anywhere * making
	// them fall through to the next appropriate outdented node (or the end)

	// Firstly turn all the alias gotos into final gotos
	for (auto& Alias : AliasedGotoLabels)
	{
		if (int* pIdx = GotoLabelList.Find(Alias.Value))
		{
			GotoLabelList.Add(Alias.Key, *pIdx);
		}
	}
	AliasedGotoLabels.Reset();
	

	// We go through top-to-bottom, which is the order of lines in the file as well
	// We don't need to cascade for this
	for (int i = 0; i < Nodes.Num(); ++i)
	{
		auto& Node = Nodes[i];
		if (Node.Edges.IsEmpty())
		{
			if (Node.NodeType == ESUDSParsedNodeType::Goto)
			{
				// Try to resolve goto now that we've parsed all labels
				// We don't actually create edges here, the label is enough so long as it leads somewhere
				// Check aliases first
				FString Label = Node.SpeakerOrGotoLabel;
				// Special case 'end' which needs no further checking
				if (Label != EndGotoLabel)
				{
					const int GotoNodeIdx = GetGotoTargetNodeIndex(Node.SpeakerOrGotoLabel);
					if (GotoNodeIdx == -1)
					{
						if (!bSilent)
							UE_LOG(LogSUDSImporter, Warning, TEXT("Error in %s: Goto label '%s' was not found, references to it will goto End"), *NameForErrors, *Node.SpeakerOrGotoLabel)
					}
				}
			}
			else
			{
				// Find the next node which is at a higher indent level than this
				const auto FallthroughIdx = FindNextOutdentedNodeIndex(i+1, Node.OriginalIndent, Node.TreePath);
				if (Nodes.IsValidIndex(FallthroughIdx))
				{
					Node.Edges.Add(FSUDSParsedEdge(FallthroughIdx));
				}
				else
				{
					// If no node to fallthrough to, default will be to end
				}
			}
		}
		else
		{
			for (auto& Edge : Node.Edges)
			{
				if (!Nodes.IsValidIndex(Edge.TargetNodeIdx))
				{
					// Usually this is a choice line without anything under it, or a condition with nothing in it
					const auto FallthroughIdx = FindNextOutdentedNodeIndex(i+1, Node.OriginalIndent, Node.TreePath);
					if (Nodes.IsValidIndex(FallthroughIdx))
					{
						Edge.TargetNodeIdx = FallthroughIdx;
					}
					else
					{
						// If no node to fallthrough to, goto end
						Edge.TargetNodeIdx = -1;
					}
				}
			}
		}
	}
}

int FSUDSScriptImporter::FindNextOutdentedNodeIndex(int StartNodeIndex, int IndentLessThan, const FString& FromPath)
{
	// In order to be a valid fallthrough, also needs to be on the same choice (or select) path
	// E.g. it's possible to have:
	// 
	// * Choice (C1)
	//		* Nested choice (C1.1)
	//			Fallthrough from here (F1)
	// * Choice (C2)
	//		Do NOT fallthrough to here (T1)
	// Fallthrough to here instead (T2)
	//
	// Just testing the indent would fallthrough to T1 which is incorrect because it's not on the same choice path
	// We need to check that the fallthrough point is on the same choice path (not just a common parent)
	// Fallthrough from is on path /C1/C1.1
	// Point T1 is on /C2 which is NOT a subset of /C1/C1.1 so not OK
	// Point T2 is on /, which is a subset of /C1/C1/1 so OK
	//
	// Also need to deal with this case:
	//
	// * Choice (C1)
	//		* Nested choice (C1.1)
	//			Fallthrough from here (F1)
	//		Fallthrough to here (T1) then from here (F2)
	// * Choice (C2)
	//		Do NOT fallthrough to here (T2)
	// Finally fall through to here (T3)
	//
	// In this case, fallthrough should first go to T1, then to T3
	//  - Fallthrough F1 is on path /C1/C1.1
	//  - Point T1 is on /C1 which IS a subset of /C1/C1.1 so OK
	//  - Fallthrough F2 is on path /C1
	//  - Point T2 is on /C2 which is NOT a subset of /C1 so not OK 
	//  - Point T3 is on / which is a subset of /C1 so OK

	// We'll form these paths just from node indexes rather than C1/C2 etc. Nesting can be for a choice or a select
	for (int i = StartNodeIndex; i < Nodes.Num(); ++i)
	{
		auto N = Nodes[i];
		if (N.OriginalIndent < IndentLessThan &&
			FromPath.StartsWith(N.TreePath))
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

int FSUDSScriptImporter::GetGotoTargetNodeIndex(const FString& InLabel)
{
	FString Label = InLabel;
	if (FString* AliasLabel = AliasedGotoLabels.Find(Label))
	{
		Label = *AliasLabel;
	}
							
	// Resolve using goto list
	if (const int *pGotoIdx = GotoLabelList.Find(Label))
	{
		return *pGotoIdx;
	}

	return -1;
	
}

void FSUDSScriptImporter::PopulateAsset(USUDSScript* Asset)
{
	// This is only called if the parsing was successful
	// Populate the runtime asset
	TArray<USUDSScriptNode*> *pOutNodes = nullptr;
	TMap<FName, int> *pOutLabels = nullptr;
	Asset->StartImport(&pOutNodes, &pOutLabels);

	if (pOutNodes && pOutLabels)
	{
		TArray<int> IndexRemap;
		int OutIndex = 0;
		// First pass, create all the nodes
		for (const auto& InNode : Nodes)
		{
			// Gotos are dealt with in the node that references them, so ignore them
			// We're going to be removing Goto nodes in the parse structure, because they were useful while parsing
			// (letting you fallthrough to a goto node) but in the final runtime we just want them to be edges
			// So firstly we need to figure out what the indexes of other nodes are going to be with them removed
			if (InNode.NodeType == ESUDSParsedNodeType::Goto)
			{
				// note that this one goes nowhere, and don't increment dest index
				IndexRemap.Add(-1);
			}
			else
			{
				IndexRemap.Add(OutIndex++);
				
				USUDSScriptNode* Node = NewObject<USUDSScriptNode>(Asset);
				switch (InNode.NodeType)
				{
				case ESUDSParsedNodeType::Text:
					// TODO: localise text
					Node->InitText(InNode.SpeakerOrGotoLabel, InNode.Text);
					break;
				case ESUDSParsedNodeType::Choice:
					Node->InitChoice();
					break;
				case ESUDSParsedNodeType::Select:
					Node->InitSelect();
					break;
				case ESUDSParsedNodeType::Goto:
				default: ;
				}

				pOutNodes->Add(Node);

			}
		}

		// Second pass, create edges between nodes now that we know where everything is
		for (int i = 0; i < Nodes.Num(); ++i)
		{
			const FSUDSParsedNode& InNode = Nodes[i];
			if (InNode.NodeType != ESUDSParsedNodeType::Goto)
			{
				USUDSScriptNode* Node = (*pOutNodes)[IndexRemap[i]];
				// Edges
				if (InNode.Edges.Num() == 0)
				{
					// This normally happens with the final node in the script
					// Make it an edge to nullptr for consistency
					Node->AddEdge(FSUDSScriptEdge(nullptr, ESUDSScriptEdgeNavigation::Explicit));
				}
				else
				{
					for (auto& InEdge : InNode.Edges)
					{
						if (const FSUDSParsedNode *InTargetNode = GetNode(InEdge.TargetNodeIdx))
						{
							FSUDSScriptEdge NewEdge;
							
							// TODO: localise edge text
							NewEdge.TempText = InEdge.Text;
							
							if (InTargetNode->NodeType == ESUDSParsedNodeType::Goto)
							{
								// Resolve GOTOs immediately, point them directly at node goto points to
								int Idx = GetGotoTargetNodeIndex(InTargetNode->SpeakerOrGotoLabel);
								// -1 means "Goto end", leave target null in that case
								if (Idx != -1)
								{
									int NewTargetIndex = IndexRemap[Idx];
									NewEdge.TargetNode = (*pOutNodes)[NewTargetIndex];
								}
							}
							else
							{
								int NewTargetIndex = IndexRemap[InEdge.TargetNodeIdx];
								NewEdge.TargetNode = (*pOutNodes)[NewTargetIndex];
							}

							if (InTargetNode->NodeType == ESUDSParsedNodeType::Choice)
							{
								NewEdge.Navigation = ESUDSScriptEdgeNavigation::Combine;
							}
							else
							{
								// TODO: support other navigation types like auto, timer based
								NewEdge.Navigation = ESUDSScriptEdgeNavigation::Explicit;
							}

							Node->AddEdge(NewEdge);
						}
						
					}
				}
			}
		}

		// Add labels, so that dialogue can be entered at any label
		// Aliases have already been resolved
		for (auto& Elem : GotoLabelList)
		{
			int NewIndex = IndexRemap[Elem.Value];
			pOutLabels->Add(FName(Elem.Key), NewIndex);
		}

		Asset->FinishImport();
	}
	
}

PRAGMA_ENABLE_OPTIMIZATION
