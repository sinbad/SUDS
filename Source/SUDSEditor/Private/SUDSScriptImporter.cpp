#include "SUDSScriptImporter.h"

#include "SUDSScript.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeSet.h"
#include "SUDSScriptNodeText.h"
#include "Internationalization/Regex.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Misc/DefaultValueHelper.h"

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
	HeaderTree.Reset();
	BodyTree.Reset();
	bHeaderDone = false;
	bHeaderInProgress = false;
	bool bImportedOK = true;
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

	ConnectRemainingNodes(HeaderTree, NameForErrors, bSilent);
	ConnectRemainingNodes(BodyTree, NameForErrors, bSilent);

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
		return ParseHeaderLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, bSilent);
	}

	// Process body
	return ParseBodyLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, bSilent);
	
}

bool FSUDSScriptImporter::ParseHeaderLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent)
{
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:00: HEADER: %s"), LineNo, *FString(Line));

	// Header can still have indenting, only a very limited set of functions though (conditionals)
	while (HeaderTree.IndentLevelStack.Num() > 1 &&
		IndentLevel < HeaderTree.IndentLevelStack.Top().ThresholdIndent)
	{
		// Pop as much from the stack as necessary to return to this indent level
		PopIndent(HeaderTree);
	}

	if (HeaderTree.IndentLevelStack.IsEmpty())
	{
		// Must be the first body line encountered. Add 1 indent level for the root
		PushIndent(HeaderTree, -1, 0, "");
	}
	
	// Header lines include set / conditionals
	// We ignore every other type of line
	if (Line.StartsWith(TEXT('[')))
	{
		bool bParsed = ParseSetLine(Line, HeaderTree, 0, LineNo, NameForErrors, bSilent);
	}
	
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

	while (BodyTree.IndentLevelStack.Num() > 1 &&
		IndentLevel < BodyTree.IndentLevelStack.Top().ThresholdIndent)
	{
		// Pop as much from the stack as necessary to return to this indent level
		PopIndent(BodyTree);
	}

	if (BodyTree.IndentLevelStack.IsEmpty())
	{
		// Must be the first body line encountered. Add 1 indent level for the root
		PushIndent(BodyTree, -1, 0, "");
	}

	if (Line.StartsWith(TEXT('*')))
	{
		return ParseChoiceLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	else if (Line.StartsWith(TEXT(':')))
	{
		return ParseGotoLabelLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	else if (Line.StartsWith(TEXT('[')))
	{
		bool bParsed = ParseConditionalLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseGotoLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseSetLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
		if (!bParsed)
			bParsed = ParseEventLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);

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
		return ParseTextLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, bSilent);
	}
	


	return true;
	
	
}


bool FSUDSScriptImporter::ParseChoiceLine(const FStringView& Line,
                                          FSUDSScriptImporter::ParsedTree& Tree,
                                          int IndentLevel,
                                          int LineNo,
                                          const FString&
                                          NameForErrors,
                                          bool bSilent)
{
	if (Line.StartsWith('*'))
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: CHOICE: %s"), LineNo, IndentLevel, *FString(Line));
		
		auto& Ctx = Tree.IndentLevelStack.Top();

		// If the current indent context node is NOT a choice, create a choice node, and connect to previous node (using pending edge if needed)
		if (!Tree.Nodes.IsValidIndex(Ctx.LastNodeIdx) ||
			Tree.Nodes[Ctx.LastNodeIdx].NodeType != ESUDSParsedNodeType::Choice)
		{
			// Last node was not a choice node, so to add edge for this choice we first need to create the choice node
			AppendNode(Tree, FSUDSParsedNode(ESUDSParsedNodeType::Choice, IndentLevel, LineNo));
		}
		if (Tree.EdgeInProgress)
		{
			// Must already have been a choice node but previous pending edge wasn't resolved
			// This means it's a fallthrough, will be resolved at end
			Tree.EdgeInProgress = nullptr;			
		}

		auto& ChoiceNode = Tree.Nodes[Ctx.LastNodeIdx];
		
		// Inside each choice, everything should be indented at least as much as 1 character inside the *
		// We provide the edge with context C001, C002 etc for fallthrough
		PushIndent(Tree, Ctx.LastNodeIdx, IndentLevel + 1, FString::Printf(TEXT("C%03d"), ++ChoiceUniqueId));
		
		// Add a pending edge, with the choice text
		// Following things fill in the edge details, the next node to be parsed will finalise the destination
		FString ChoiceTextID;
		auto ChoiceTextView = Line.SubStr(1, Line.Len() - 1).TrimStart();
		RetrieveAndRemoveOrGenerateTextID(ChoiceTextView, ChoiceTextID);
		const int EdgeIdx = ChoiceNode.Edges.Add(FSUDSParsedEdge(-1, LineNo, FString(ChoiceTextView), ChoiceTextID));
		Tree.EdgeInProgress = &ChoiceNode.Edges[EdgeIdx];
		
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseConditionalLine(const FStringView& Line,
                                               FSUDSScriptImporter::ParsedTree& Tree,
                                               int IndentLevel,
                                               int LineNo,
                                               const FString&
                                               NameForErrors,
                                               bool bSilent)
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

bool FSUDSScriptImporter::ParseGotoLabelLine(const FStringView& Line,
                                             FSUDSScriptImporter::ParsedTree& Tree,
                                             int IndentLevel,
                                             int LineNo,
                                             const FString&
                                             NameForErrors,
                                             bool bSilent)
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
			Tree.PendingGotoLabels.Add(Label);
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

bool FSUDSScriptImporter::ParseGotoLine(const FStringView& Line,
                                        FSUDSScriptImporter::ParsedTree& Tree,
                                        int IndentLevel,
                                        int LineNo,
                                        const FString&
                                        NameForErrors,
                                        bool bSilent)
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
		const auto& Ctx = Tree.IndentLevelStack.Top();
		// A goto is an edge from the current node to another node
		// That means if we had pending labels that didn't hit a node before now, they're just aliases to THIS label
		for (auto PendingLabel : Tree.PendingGotoLabels)
		{
			Tree.AliasedGotoLabels.Add(PendingLabel, Label);
		}
		Tree.PendingGotoLabels.Reset();
		AppendNode(Tree, FSUDSParsedNode(Label, IndentLevel, LineNo));
		return true;
	}
	if (!bSilent)
		UE_LOG(LogSUDSImporter, Error, TEXT("Error in %s line %d: malformed goto line"), *NameForErrors, LineNo);
	return false;
}

bool FSUDSScriptImporter::ParseSetLine(const FStringView& InLine,
                                       FSUDSScriptImporter::ParsedTree& Tree,
                                       int IndentLevel,
                                       int LineNo,
                                       const FString&
                                       NameForErrors,
                                       bool bSilent)
{
	// Attempt to find existing text ID, for string literals
	// For multiple lines, may not be present until last line (but in fact can be on any line)
	// We generate anyway, because it can be overriden by later lines, but makes sure we have one always
	FString TextID;
	FStringView Line = InLine;
	RetrieveAndRemoveTextID(Line, TextID);
	
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	const FRegexPattern SetPattern(TEXT("^\\[set\\s+(\\S+)\\s+(.+)\\]$"));
	FRegexMatcher SetRegex(SetPattern, LineStr);
	if (SetRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: SET   : %s"), LineNo, IndentLevel, *FString(Line));

		FString Name = SetRegex.GetCaptureGroup(1);
		FString ValueStr = SetRegex.GetCaptureGroup(2).TrimStartAndEnd(); // trim because capture accepts spaces in quotes

		// Parse literal argument
		// TODO: support references to other variables, expressions
		FSUDSValue LiteralValue;
		if (ParseLiteral(ValueStr, LiteralValue))
		{
			if (LiteralValue.GetType() == ESUDSValueType::Text)
			{
				// Text must be localised
				if (TextID.IsEmpty())
				{
					TextID = GenerateTextID(InLine);
				}
				AppendNode(Tree, FSUDSParsedNode(Name, LiteralValue, TextID, IndentLevel, LineNo));
			}
			else
			{
				AppendNode(Tree, FSUDSParsedNode(Name, LiteralValue, IndentLevel, LineNo));
			}
			return true;
		}
		
		if (!bSilent)
			UE_LOG(LogSUDSImporter, Error, TEXT("Error in %s line %d: malformed set line"), *NameForErrors, LineNo);

	}
	return false;
}

bool FSUDSScriptImporter::ParseLiteral(const FString& ValueStr, FSUDSValue& OutVal)
{
	// Try Boolean first since only 2 options
	{
		if (ValueStr.Compare("true", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(true);
			return true;
		}
		if (ValueStr.Compare("false", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(false);
			return true;
		}
	}
	// Try gender
	{
		if (ValueStr.Compare("masculine", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Masculine);
			return true;
		}
		if (ValueStr.Compare("feminine", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Feminine);
			return true;
		}
		if (ValueStr.Compare("neuter", ESearchCase::IgnoreCase) == 0)
		{
			OutVal = FSUDSValue(ETextGender::Neuter);
			return true;
		}
	}
	// Try quoted text (will be localised later in asset conversion)
	{
		const FRegexPattern Pattern(TEXT("^\\\"([^\\\"]*)\\\"$"));
		FRegexMatcher Regex(Pattern, ValueStr);
		if (Regex.FindNext())
		{
			const FString Val = Regex.GetCaptureGroup(1);
			OutVal = FSUDSValue(FText::FromString(Val));
			return true;
		}
	}
	// Try Numbers
	{
		float FloatVal;
		int IntVal;
		// look for int first; anything with a decimal point will fail
		if (FDefaultValueHelper::ParseInt(ValueStr, IntVal))
		{
			OutVal = FSUDSValue(IntVal);	
			return true;
		}
		if (FDefaultValueHelper::ParseFloat(ValueStr, FloatVal))
		{
			OutVal = FSUDSValue(FloatVal);	
			return true;
		}
	}

	return false;
	
}

bool FSUDSScriptImporter::ParseEventLine(const FStringView& Line,
                                         FSUDSScriptImporter::ParsedTree& Tree,
                                         int IndentLevel,
                                         int LineNo,
                                         const FString&
                                         NameForErrors,
                                         bool bSilent)
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

bool FSUDSScriptImporter::ParseTextLine(const FStringView& InLine,
                                        FSUDSScriptImporter::ParsedTree& Tree,
                                        int IndentLevel,
                                        int LineNo,
                                        const FString&
                                        NameForErrors,
                                        bool bSilent)
{
	auto& Ctx = Tree.IndentLevelStack.Top();

	// Attempt to find existing text ID
	// For multiple lines, may not be present until last line (but in fact can be on any line)
	// We generate anyway, because it can be overriden by later lines, but makes sure we have one always
	FString TextID;
	FStringView Line = InLine;
	RetrieveAndRemoveOrGenerateTextID(Line, TextID);
	
	const FString LineStr(Line);
	const FRegexPattern SpeakerPattern(TEXT("^(\\w+)\\:\\s*(.+)$"));
	FRegexMatcher SpeakerRegex(SpeakerPattern, LineStr);
	if (SpeakerRegex.FindNext())
	{
		// OK this is a speaker line, in which case this is a new text node
		const FString Speaker = SpeakerRegex.GetCaptureGroup(1);
		const FString Text = SpeakerRegex.GetCaptureGroup(2);
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: TEXT  : %s"), LineNo, IndentLevel, *FString(Line));
		// New text node
		// Text nodes can never introduce another indent context
		// We've already backed out to the outer indent in caller
		AppendNode(Tree, FSUDSParsedNode(Speaker, Text, TextID, IndentLevel, LineNo));

		ReferencedSpeakers.AddUnique(Speaker);
		
		return true;
	}

	// If we fell through, this line is appended to the last text node
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: TEXT+ : %s"), LineNo, IndentLevel, *FString(Line));
	if (Tree.Nodes.IsValidIndex(Ctx.LastNodeIdx))
	{
		auto& Node = Tree.Nodes[Ctx.LastNodeIdx];
		if (Node.NodeType == ESUDSParsedNodeType::Text)
		{
			Node.Text.Appendf(TEXT("\n%s"), *LineStr);

			// TextID may have been found in the line continuation
			// We override the generated one silently in this case (can cause gaps in auto numbering but that's ok)
			if (!TextID.IsEmpty())
			{
				Node.TextID = TextID;
			}
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

void FSUDSScriptImporter::RetrieveAndRemoveOrGenerateTextID(FStringView& InOutLine, FString& OutTextID)
{
	if (!RetrieveAndRemoveTextID(InOutLine, OutTextID))
	{
		OutTextID = GenerateTextID(InOutLine);
	}
}

bool FSUDSScriptImporter::RetrieveAndRemoveTextID(FStringView& InOutLine, FString& OutTextID)
{
	// Find any TextID in the line, and if found, remove it and move it to OutTextID
	// Also set the last text ID number from this so we never generate duplicates
	const FString LineStr(InOutLine);
	const FRegexPattern TextIDPattern(TEXT("(\\@([0-9a-fA-F]+)\\@)"));
	FRegexMatcher TextIDRegex(TextIDPattern, LineStr);
	if (TextIDRegex.FindNext())
	{
		OutTextID = TextIDRegex.GetCaptureGroup(1);
		// Chop the incoming string to the left of the TextID
		InOutLine = InOutLine.Left(TextIDRegex.GetCaptureGroupBeginning(1));
		// Also trim right
		InOutLine = InOutLine.TrimEnd();
		// FDefaultValueHelper::ParseInt requires an "0x" prefix but we're not using that
		// Plus does extra checking we don't need
		TextIDHighestNumber = FCString::Strtoi(*TextIDRegex.GetCaptureGroup(2), nullptr, 16);
		return true;
	}

	return false;

}

FString FSUDSScriptImporter::GenerateTextID(const FStringView& Line)
{
	// Generate a new text ID just based on ascending numbers
	// We don't actually base this on the line but we have it for future possible use
	// Since it's a string, format exactly as in the sud file 
	return FString::Printf(TEXT("@%x@"), ++TextIDHighestNumber);
}

FString FSUDSScriptImporter::GetCurrentTreePath(FSUDSScriptImporter::ParsedTree& Tree)
{
	// This is just a path of all the choice / select nodes AND their edges leading to this point, for fallthrough
	// * Choice (/C000/)
	//		* Nested choice (/C000/C001/)
	//			Fallthrough from here
	// * Choice (/C002/C003/)
	//		Do NOT fallthrough to here
	// Fallthrough to here instead (/)

	FStringBuilderBase B;
	for (auto Indent : Tree.IndentLevelStack)
	{
		B.Appendf(TEXT("%s%s"), *Indent.PathEntry, *TreePathSeparator);
	}
	return B.ToString();
}

int FSUDSScriptImporter::AppendNode(FSUDSScriptImporter::ParsedTree& Tree, const FSUDSParsedNode& NewNode)
{
	auto& Ctx = Tree.IndentLevelStack.Top();

	const int NewIndex = Tree.Nodes.Add(NewNode);

	// Set the tree path of the node (post-add)
	{
		auto& N = Tree.Nodes[NewIndex];
		N.TreePath = GetCurrentTreePath(Tree);
	}
	
	if (Tree.Nodes.IsValidIndex(Ctx.LastNodeIdx))
	{
		// Append this node onto the last one
		auto& PrevNode = Tree.Nodes[Ctx.LastNodeIdx];
		// Use pending edge if present; that could be because this is under a choice node, or a condition
		if (Tree.EdgeInProgress)
		{
			Tree.EdgeInProgress->TargetNodeIdx = NewIndex;
		}
		else
		{
			// Auto-connect new nodes to previous nodes
			// Valid for nodes with only one output node
			// With choices / selects, we should be doing it via pending edges
			// E.g. choice nodes get edges created for choice options, select nodes for conditions
			// A new node with no pending edge following any other type may be connected via fallthrough at
			// the end of parsing
			if (PrevNode.NodeType == ESUDSParsedNodeType::Text ||
				PrevNode.NodeType == ESUDSParsedNodeType::SetVariable)
			{
				PrevNode.Edges.Add(FSUDSParsedEdge(NewIndex, NewNode.SourceLineNo));
			}
		}
		Tree.EdgeInProgress = nullptr;
	}

	// All goto labels in scope at this point now point to this node
	for (auto Label : Tree.PendingGotoLabels)
	{
		Tree.GotoLabelList.Add(Label, NewIndex);
	}
	Tree.PendingGotoLabels.Reset();

	Ctx.LastNodeIdx = NewIndex;
	Ctx.ThresholdIndent = FMath::Min(Ctx.ThresholdIndent, NewNode.OriginalIndent);
	

	return NewIndex;
}

void FSUDSScriptImporter::PopIndent(FSUDSScriptImporter::ParsedTree& Tree)
{
	Tree.IndentLevelStack.Pop();
	// We *could* reset the pending goto list if there are labels at higher indents than current that never resolved to a node
	// but I'm choosing not to right now and letting them fall through
}

void FSUDSScriptImporter::PushIndent(FSUDSScriptImporter::ParsedTree& Tree, int NodeIdx, int Indent, const FString& Path)
{
	Tree.IndentLevelStack.Push(IndentContext(NodeIdx, Indent, Path));

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

void FSUDSScriptImporter::ConnectRemainingNodes(FSUDSScriptImporter::ParsedTree& Tree, const FString& NameForErrors, bool bSilent)
{
	// Now we go through all nodes, resolving gotos, and finding links that don't go anywhere * making
	// them fall through to the next appropriate outdented node (or the end)

	// Firstly turn all the alias gotos into final gotos
	for (auto& Alias : Tree.AliasedGotoLabels)
	{
		if (int* pIdx = Tree.GotoLabelList.Find(Alias.Value))
		{
			Tree.GotoLabelList.Add(Alias.Key, *pIdx);
		}
	}
	Tree.AliasedGotoLabels.Reset();
	

	// We go through top-to-bottom, which is the order of lines in the file as well
	// We don't need to cascade for this
	for (int i = 0; i < Tree.Nodes.Num(); ++i)
	{
		auto& Node = Tree.Nodes[i];
		if (Node.Edges.IsEmpty())
		{
			if (Node.NodeType == ESUDSParsedNodeType::Goto)
			{
				// Try to resolve goto now that we've parsed all labels
				// We don't actually create edges here, the label is enough so long as it leads somewhere
				// Check aliases first
				FString Label = Node.Identifier;
				// Special case 'end' which needs no further checking
				if (Label != EndGotoLabel)
				{
					const int GotoNodeIdx = GetGotoTargetNodeIndex(Tree, Node.Identifier);
					if (GotoNodeIdx == -1)
					{
						if (!bSilent)
							UE_LOG(LogSUDSImporter, Warning, TEXT("Error in %s line %d: Goto label '%s' was not found, references to it will goto End"), *NameForErrors, Node.SourceLineNo, *Node.Identifier)
					}
				}
			}
			else
			{
				// Find the next node which is at a higher indent level than this
				const auto FallthroughIdx = FindNextOutdentedNodeIndex(Tree, i+1, Node.OriginalIndent, Node.TreePath);
				if (Tree.Nodes.IsValidIndex(FallthroughIdx))
				{
					Node.Edges.Add(FSUDSParsedEdge(FallthroughIdx, Node.SourceLineNo));
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
				if (!Tree.Nodes.IsValidIndex(Edge.TargetNodeIdx))
				{
					// Usually this is a choice line without anything under it, or a condition with nothing in it
					const auto FallthroughIdx = FindNextOutdentedNodeIndex(Tree, i+1, Node.OriginalIndent, Node.TreePath);
					if (Tree.Nodes.IsValidIndex(FallthroughIdx))
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

int FSUDSScriptImporter::FindNextOutdentedNodeIndex(FSUDSScriptImporter::ParsedTree& Tree, int StartNodeIndex, int IndentLessThan, const FString& FromPath)
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
	for (int i = StartNodeIndex; i < Tree.Nodes.Num(); ++i)
	{
		auto N = Tree.Nodes[i];
		if (N.OriginalIndent < IndentLessThan &&
			FromPath.StartsWith(N.TreePath))
		{
			return i;
		}
		
	}

	return -1;
}

const FSUDSParsedNode* FSUDSScriptImporter::GetNode(const FSUDSScriptImporter::ParsedTree& Tree, int Index)
{
	if (Tree.Nodes.IsValidIndex(Index))
	{
		return &Tree.Nodes[Index];
	}

	return nullptr;
}

const FSUDSParsedNode* FSUDSScriptImporter::GetHeaderNode(int Index)
{
	return GetNode(HeaderTree, Index);
}

const FSUDSParsedNode* FSUDSScriptImporter::GetNode(int Index)
{
	return GetNode(BodyTree, Index);
}

int FSUDSScriptImporter::GetGotoTargetNodeIndex(const FString& InLabel)
{
	// Assume Body for this public version
	return GetGotoTargetNodeIndex(BodyTree, InLabel);
}

int FSUDSScriptImporter::GetGotoTargetNodeIndex(const ParsedTree& Tree, const FString& InLabel)
{
	FString Label = InLabel;
	if (const FString* AliasLabel = Tree.AliasedGotoLabels.Find(Label))
	{
		Label = *AliasLabel;
	}
							
	// Resolve using goto list
	if (const int *pGotoIdx = Tree.GotoLabelList.Find(Label))
	{
		return *pGotoIdx;
	}

	return -1;
	
}

void FSUDSScriptImporter::PopulateAsset(USUDSScript* Asset, UStringTable* StringTable)
{
	// This is only called if the parsing was successful
	// Populate the runtime asset
	TArray<USUDSScriptNode*> *pOutNodes = nullptr;
	TArray<USUDSScriptNode*> *pOutHeaderNodes = nullptr;
	TMap<FName, int> *pOutLabels = nullptr;
	TMap<FName, int> *pOutHeaderLabels = nullptr;
	TArray<FString> *pOutSpeakers = nullptr;
	Asset->StartImport(&pOutNodes, &pOutHeaderNodes, &pOutLabels, &pOutHeaderLabels, &pOutSpeakers);

	pOutSpeakers->Append(ReferencedSpeakers);

	PopulateAssetFromTree(Asset, HeaderTree, pOutHeaderNodes, pOutHeaderLabels, StringTable);
	PopulateAssetFromTree(Asset, BodyTree, pOutNodes, pOutLabels, StringTable);

	Asset->FinishImport();
}

void FSUDSScriptImporter::PopulateAssetFromTree(USUDSScript* Asset,
                                                const FSUDSScriptImporter::ParsedTree& Tree,
                                                TArray<USUDSScriptNode*>* pOutNodes,
                                                TMap<FName, int>* pOutLabels,
                                                UStringTable* StringTable)
{
	if (pOutNodes && pOutLabels)
	{
		TArray<int> IndexRemap;
		int OutIndex = 0;
		// First pass, create all the nodes
		for (const auto& InNode : Tree.Nodes)
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
				
				USUDSScriptNode* Node = nullptr;
				switch (InNode.NodeType)
				{
				case ESUDSParsedNodeType::Text:
					{
						StringTable->GetMutableStringTable()->SetSourceString(InNode.TextID, InNode.Text);
						auto TextNode = NewObject<USUDSScriptNodeText>(Asset);
						TextNode->Init(InNode.Identifier, FText::FromStringTable (StringTable->GetStringTableId(), InNode.TextID));
						Node = TextNode;
						break;
					}
				case ESUDSParsedNodeType::Choice:
					{
						auto ChoiceNode = NewObject<USUDSScriptNode>(Asset);
						ChoiceNode->InitChoice();
						Node = ChoiceNode;
						break;
					}
				case ESUDSParsedNodeType::Select:
					{
						auto SelectNode = NewObject<USUDSScriptNode>(Asset);
						SelectNode->InitSelect();
						Node = SelectNode;
						break;
					}
				case ESUDSParsedNodeType::SetVariable:
					{
						auto SetNode = NewObject<USUDSScriptNodeSet>(Asset);
						// TODO support expressions not just literals
						SetNode->Init(InNode.Identifier, InNode.VarLiteral);
						Node = SetNode;
						break;
					}
				case ESUDSParsedNodeType::Goto:
				default: ;
				}

				pOutNodes->Add(Node);

			}
		}

		// Second pass, create edges between nodes now that we know where everything is
		for (int i = 0; i < Tree.Nodes.Num(); ++i)
		{
			const FSUDSParsedNode& InNode = Tree.Nodes[i];
			if (InNode.NodeType != ESUDSParsedNodeType::Goto)
			{
				USUDSScriptNode* Node = (*pOutNodes)[IndexRemap[i]];
				// Edges
				if (InNode.Edges.Num() == 0)
				{
					// This normally happens with the final node in the script
					// Make it an edge to nullptr for consistency
					Node->AddEdge(FSUDSScriptEdge(nullptr));
				}
				else
				{
					for (auto& InEdge : InNode.Edges)
					{
						FSUDSScriptEdge NewEdge;

						if (!InEdge.TextID.IsEmpty() && !InEdge.Text.IsEmpty())
						{
							StringTable->GetMutableStringTable()->SetSourceString(InEdge.TextID, InEdge.Text);
							NewEdge.SetText(FText::FromStringTable(StringTable->GetStringTableId(), InEdge.TextID));
						}
						
						if (const FSUDSParsedNode *InTargetNode = GetNode(Tree, InEdge.TargetNodeIdx))
						{
							
							if (InTargetNode->NodeType == ESUDSParsedNodeType::Goto)
							{
								// Resolve GOTOs immediately, point them directly at node goto points to
								int Idx = GetGotoTargetNodeIndex(Tree, InTargetNode->Identifier);
								// -1 means "Goto end", leave target null in that case
								if (Idx != -1)
								{
									int NewTargetIndex = IndexRemap[Idx];
									NewEdge.SetTargetNode((*pOutNodes)[NewTargetIndex]);
								}
							}
							else
							{
								int NewTargetIndex = IndexRemap[InEdge.TargetNodeIdx];
								NewEdge.SetTargetNode((*pOutNodes)[NewTargetIndex]);
							}

						}

						Node->AddEdge(NewEdge);

					}
				}
			}
		}

		// Add labels, so that dialogue can be entered at any label
		// Aliases have already been resolved
		for (auto& Elem : Tree.GotoLabelList)
		{
			int NewIndex = IndexRemap[Elem.Value];
			pOutLabels->Add(FName(Elem.Key), NewIndex);
		}

	}
	
}

PRAGMA_ENABLE_OPTIMIZATION
