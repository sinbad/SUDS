// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptImporter.h"

#include "SUDSEditorSettings.h"
#include "SUDSExpression.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeEvent.h"
#include "SUDSScriptNodeGosub.h"
#include "SUDSScriptNodeSet.h"
#include "SUDSScriptNodeText.h"
#include "Internationalization/Regex.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

class USUDSEditorSettings;
const FString FSUDSScriptImporter::EndGotoLabel = "end";
const FString FSUDSScriptImporter::TreePathSeparator = "/";

DEFINE_LOG_CATEGORY(LogSUDSImporter)


bool FSUDSScriptImporter::ImportFromBuffer(const TCHAR *Start, int32 Length, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent)
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
	PersistentMetadata.Empty();
	TransientMetadata.Empty();
	bHeaderDone = false;
	bHeaderInProgress = false;
	bTooLateForHeader = false;
	bool bImportedOK = true;
	ChoiceUniqueId = 0;
	TextIDHighestNumber = 0;
	bOverrideGenerateSpeakerLineForChoice.Reset();
	OverrideChoiceSpeakerID.Reset();
	ReferencedSpeakers.Empty();
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
				FStringView Line = FStringView(Start + SubstringBeginIndex, SubstringLength);
				if (!ParseLine(Line, LineNumber++, NameForErrors, Logger, bSilent))
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
		const FStringView Line = FStringView(Start + SubstringBeginIndex, SubstringLength);
		bImportedOK = ParseLine(Line, LineNumber++, NameForErrors, Logger, bSilent) && bImportedOK;
	}

	ConnectRemainingNodes(HeaderTree, NameForErrors, Logger, bSilent);
	ConnectRemainingNodes(BodyTree, NameForErrors, Logger, bSilent);
	GenerateTextIDs(HeaderTree);
	GenerateTextIDs(BodyTree);

	bImportedOK = PostImportSanityCheck(NameForErrors, Logger, bSilent) && bImportedOK;

	return bImportedOK;
	
}

bool FSUDSScriptImporter::ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent)
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

		// May be metadata in the comment though
		ParseCommentMetadataLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		
		return true;
	}

	// Check for headers
	static const FStringView HeaderPrefix(TEXT("==="));
	if (TrimmedLine.StartsWith(HeaderPrefix))
	{
		if (bHeaderDone)
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Failed to parse %s Line %d: Duplicate header section"), *NameForErrors, LineNo);
			return false;
		}
		else if (bTooLateForHeader)
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Failed to parse %s Line %d: Header section must be at start"), *NameForErrors, LineNo);
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
		return ParseHeaderLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	}

	// Process body
	return ParseBodyLine(TrimmedLine, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	
}

bool FSUDSScriptImporter::ParseCommentMetadataLine(const FStringView& Line,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	// Comment metadata starts with:
	// #= [Key:] Single Use Metadata (next line only)
	// #+ [Key:] Persistent Metadata (apply to all lines until reset)
	// [Key:] is optional; if omitted the key is "Comment"
	// Persistent Metadata is reset when:
	//   - The same key is set again (can be set to blank to reset to empty)
	//   - A line that is more outdented than the source of the key is encountered

	FString LineStr(Line);
	const FRegexPattern MetaPattern(TEXT("^#([\\=\\+])\\s*(?:(\\S*)\\s*:\\s*)?(.*)$"));
	FRegexMatcher MetaRegex(MetaPattern, LineStr);
	if (MetaRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: META  : %s"), LineNo, IndentLevel, *FString(Line));

		const bool bIsPersistent = MetaRegex.GetCaptureGroup(1) == "+";
		// There is no "count" of capture groups, test highest to detect if key used
		FString KeyStr = MetaRegex.GetCaptureGroup(2);
		const FName Key = FName(KeyStr.IsEmpty() ? "Comment" :KeyStr);
		const FString Value = MetaRegex.GetCaptureGroup(3).TrimStartAndEnd();

		if (bIsPersistent)
		{
			TArray<ParsedMetadata>* pStack = PersistentMetadata.Find(Key);
			if (!pStack)
			{
				// Only bother creating if non-empty
				// If we find a blank and there's a stack there already, we do add an entry since blank overrides others in scope
				if (!Value.IsEmpty())
				{
					pStack = &PersistentMetadata.Add(Key);
				}
			}

			if (pStack)
			{
				// First we need to check if this line is less or equal indented; if so we have to strip out existing stack items
				while (!pStack->IsEmpty() && IndentLevel <= pStack->Top().IndentLevel)
				{
					pStack->Pop();
				}
				pStack->Push(ParsedMetadata(Key, Value, IndentLevel));
			}
		}
		else
		{
			if (Value.IsEmpty())
			{
				// Reset
				TransientMetadata.Remove(Key);
			}
			else
			{
				TransientMetadata.Add(Key, ParsedMetadata(Key, Value, IndentLevel));
			}
			
		}
		return true;
	}

	return false;
	
}

TMap<FName, FString> FSUDSScriptImporter::GetTextMetadataForNextEntry(int CurrentLineIndent)
{
	TMap<FName, FString> Ret;

	// For each key
	for (auto It = PersistentMetadata.CreateIterator(); It; ++It)
	{
		auto& Stack = It->Value;
		// Use top of stack, so long as equally or less indented than current line
		while (!Stack.IsEmpty() && CurrentLineIndent < Stack.Top().IndentLevel)
		{
			Stack.Pop();
		}
		if (Stack.IsEmpty())
		{
			// Remove key entry if there's no values left on the stack
			It.RemoveCurrent();
		}
		else
		{
			Ret.Add(It->Key, Stack.Top().Value);
		}
	}

	// Add transient after so they override persistent
	for (auto& Pair: TransientMetadata)
	{
		// Always apply transient ones to next case, don't check indent
		Ret.Add(Pair.Key, Pair.Value.Value);
	}
	TransientMetadata.Empty();

	return Ret;
}


bool FSUDSScriptImporter::ParseHeaderLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent)
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
		bool bParsed = ParseSetLine(Line, HeaderTree, 0, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseImportSettingLine(Line, HeaderTree, 0, LineNo, NameForErrors, Logger, bSilent);;
	}
	
	return true;
}

bool FSUDSScriptImporter::ParseBodyLine(const FStringView& Line,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
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
		return ParseChoiceLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	}
	else if (Line.StartsWith(TEXT(':')))
	{
		return ParseGotoLabelLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	}
	else if (Line.StartsWith(TEXT('[')))
	{
		bool bParsed = ParseConditionalLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseGotoLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseSetLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseEventLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseGosubLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseReturnLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		if (!bParsed)
			bParsed = ParseImportSettingLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);

		if (!bParsed)
		{
			if (!bSilent)
			{
				UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: CMD  : %s"), LineNo, IndentLevel, *FString(Line));
				Logger->Logf(ELogVerbosity::Warning, TEXT("%s Line %d: Unrecognised command. Ignoring!"), *NameForErrors, LineNo);
			}
			// We still return true because we don't want to fail the entire import
		}
		return true;
	}
	else
	{
		return ParseTextLine(Line, BodyTree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	}
	


	return true;
	
	
}

bool FSUDSScriptImporter::IsLastNodeOfType(const FSUDSScriptImporter::ParsedTree& Tree, ESUDSParsedNodeType Type)
{
	auto& Ctx = Tree.IndentLevelStack.Top();
	return Tree.Nodes.IsValidIndex(Ctx.LastNodeIdx) && Tree.Nodes[Ctx.LastNodeIdx].NodeType == Type;
}

int FSUDSScriptImporter::FindLastChoiceNode(const ParsedTree& Tree, int IndentLevel)
{
	int Ret = FindLastChoiceNode(Tree, IndentLevel,Tree.Nodes.Num() - 1, GetCurrentTreeConditionalPath(Tree));
	// if (Ret == -1)
	// {
	// 	// Fallback to try to find from previous text
	// 	auto& Ctx = Tree.IndentLevelStack.Top();
	// 	
	// 	if (Tree.Nodes.IsValidIndex(Ctx.LastTextNodeIdx))
	// 	{
	// 		return FindChoiceAfterTextNode(Tree, Ctx.LastTextNodeIdx);
	// 	}
	// }
	return Ret;
}

int FSUDSScriptImporter::FindChoiceAfterTextNode(const FSUDSScriptImporter::ParsedTree& Tree, int TextNodeIdx)
{
	const auto& TextNode = Tree.Nodes[TextNodeIdx];
	if (TextNode.Edges.Num() == 1 && Tree.Nodes.IsValidIndex(TextNode.Edges[0].TargetNodeIdx))
	{
		int NextNodeIdx = TextNode.Edges[0].TargetNodeIdx;
		while (Tree.Nodes.IsValidIndex(NextNodeIdx))
		{
			auto& Node = Tree.Nodes[NextNodeIdx];
			switch (Node.NodeType)
			{
			case ESUDSParsedNodeType::Select:
			case ESUDSParsedNodeType::Text:
				// Didn't find
				return -1;
			case ESUDSParsedNodeType::Choice:
				return NextNodeIdx;
			case ESUDSParsedNodeType::SetVariable:
			case ESUDSParsedNodeType::Goto:
			case ESUDSParsedNodeType::Event:
				if (Node.Edges.Num() == 1)
				{
					NextNodeIdx = Node.Edges[0].TargetNodeIdx;
				}
				else
				{
					return -1;
				}
				break;
			default: ;
			};
		}
	}
	return -1;
	
}


int FSUDSScriptImporter::FindLastChoiceNode(const ParsedTree& Tree, int IndentLevel, int FromIndex, const FString& ConditionPath)
{
	// Scan backwards from end of tree looking for a choice node which has the same or higher indent and condition state as current
	// But abort if we hit text nodes on that path

	for (int i = FromIndex; i >= 0; --i)
	{
		auto& Node = Tree.Nodes[i];

		if (ConditionPath.StartsWith(Node.ConditionalPath) &&
			Node.NodeType == ESUDSParsedNodeType::Text &&
			Node.OriginalIndent <= IndentLevel)
		{
			// We hit a parent text node, we can't go back any further
			return -1;
		}
		// Only consider nodes on the same conditional path
		// Note: NOT containing the conditional path. We don't want to skip over an intervening select node
		if (ConditionPath == Node.ConditionalPath)
		{
			// note that we allow indents < as well as ==
			// This is so that if you choose to aesthetically indent choices it still works
			if (Node.NodeType == ESUDSParsedNodeType::Choice && Node.OriginalIndent <= IndentLevel)
			{
				return i;
			}
		}
	}
	return -1;
}

bool FSUDSScriptImporter::ParseChoiceLine(const FStringView& Line,
                                          FSUDSScriptImporter::ParsedTree& Tree,
                                          int IndentLevel,
                                          int LineNo,
                                          const FString& NameForErrors,
                                          FSUDSMessageLogger* Logger,
                                          bool bSilent)
{
	if (Line.StartsWith('*'))
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: CHOICE: %s"), LineNo, IndentLevel, *FString(Line));
		
		auto& Ctx = Tree.IndentLevelStack.Top();


		// Find a previous choice node to join with
		int ChoiceNodeIdx = - 1;
		// however, if there are pending goto labels, they MUST split the choice
		if (Tree.PendingGotoLabels.Num() == 0)
		{
			ChoiceNodeIdx = FindLastChoiceNode(Tree, IndentLevel);
		}
		
		// If the current indent context node is NOT a choice, create a choice node, and connect to previous node (using pending edge if needed)
		if (ChoiceNodeIdx == -1)
		{
			// Didn't find a choice node we can connect to, so create one
			
			// However, before we do that, if our parent is a select node, we must ensure that its parent is a choice node
			// This is just so that we have a choice node outside the select that any choices that come after the [endif]
			// can attach to. If a non-conditional choice was encountered first, it'll already be there. But if the first
			// choice was conditional, the select node will have been created first.
			// We still create a choice node here too, underneath the select, to act as a holder for potentially multiple
			// choices within this select condition
			EnsureChoiceNodeExistsAboveSelect(Tree, IndentLevel, LineNo);
			ChoiceNodeIdx = AppendNode(Tree, FSUDSParsedNode(ESUDSParsedNodeType::Choice, IndentLevel, LineNo));
		}
		if (Tree.EdgeInProgressNodeIdx != -1)
		{
			// Must already have been a choice node but previous pending edge wasn't resolved
			// This means it's a fallthrough, will be resolved at end
			Tree.EdgeInProgressNodeIdx = Tree.EdgeInProgressEdgeIdx = -1;
		}

		auto& ChoiceNode = Tree.Nodes[ChoiceNodeIdx];
		
		// Inside each choice, everything should be indented at least as much as 1 character inside the *
		// We provide the edge with context C001, C002 etc for fallthrough
		PushIndent(Tree, ChoiceNodeIdx, IndentLevel + 1, FString::Printf(TEXT("C%03d"), ++ChoiceUniqueId));

		// Do we want to generate a speaker line for this choice
		bool bGenerateSpeakerLine = false;
		FString GeneratedSpeakerID;
		if (const auto Settings = GetDefault<USUDSEditorSettings>())
		{
			bGenerateSpeakerLine = Settings->AlwaysGenerateSpeakerLinesFromChoices;
			GeneratedSpeakerID = Settings->SpeakerIdForGeneratedLinesFromChoices;
		}
		if (bOverrideGenerateSpeakerLineForChoice.IsSet())
		{
			bGenerateSpeakerLine = bOverrideGenerateSpeakerLineForChoice.GetValue();
		}
		if (OverrideChoiceSpeakerID.IsSet())
		{
			GeneratedSpeakerID = OverrideChoiceSpeakerID.GetValue();
		}
		int ChoiceTextStart = 1;
		if (Line[1] == '-')
		{
			// *- prefix, override speaker line
			bGenerateSpeakerLine = false;
			ChoiceTextStart = 2;
		}
		
		// Add a pending edge, with the choice text
		// Following things fill in the edge details, the next node to be parsed will finalise the destination
		FString ChoiceTextID;
		auto ChoiceTextView = Line.SubStr(ChoiceTextStart, Line.Len() - ChoiceTextStart).TrimStart();
		// TextID might be blank after this, but that's OK, we fix later after we know what IDs are in use
		RetrieveAndRemoveTextID(ChoiceTextView, ChoiceTextID);
		const FString ChoiceText = FString(ChoiceTextView);
		auto ChoiceTextMeta = GetTextMetadataForNextEntry(IndentLevel);
		const int EdgeIdx = ChoiceNode.Edges.Add(FSUDSParsedEdge(ChoiceNodeIdx, -1, LineNo, ChoiceText, ChoiceTextID, ChoiceTextMeta));
		Tree.EdgeInProgressNodeIdx = ChoiceNodeIdx;
		Tree.EdgeInProgressEdgeIdx = EdgeIdx;

		if (bGenerateSpeakerLine)
		{
			// We use the same text & ID so this is just one localisation entry
			Ctx.LastTextNodeIdx = AppendNode(Tree, FSUDSParsedNode(GeneratedSpeakerID, ChoiceText, ChoiceTextID, ChoiceTextMeta, IndentLevel + 1, LineNo));
			ReferencedSpeakers.AddUnique(GeneratedSpeakerID);			
		}
		return true;
	}
	return false;
}

FSUDSParsedEdge* FSUDSScriptImporter::GetEdgeInProgress(ParsedTree& Tree)
{
	if (Tree.Nodes.IsValidIndex(Tree.EdgeInProgressNodeIdx))
	{
		auto& N = Tree.Nodes[Tree.EdgeInProgressNodeIdx];
		if (N.Edges.IsValidIndex(Tree.EdgeInProgressEdgeIdx))
		{
			return &N.Edges[Tree.EdgeInProgressEdgeIdx];
		}
	}
	return nullptr;
}
void FSUDSScriptImporter::EnsureChoiceNodeExistsAboveSelect(ParsedTree& Tree, int IndentLevel, int LineNo)
{
	auto& Ctx = Tree.IndentLevelStack.Top();
	int ParentIdx = -1;
	// Inserting is going to mess up EdgeInProgress
	if (auto E = GetEdgeInProgress(Tree))
	{
		ParentIdx = E->SourceNodeIdx;
	}
	else
	{
		ParentIdx = Ctx.LastNodeIdx;
	}

	// Nothing to do if immediate parent isn't select
	if (!Tree.Nodes.IsValidIndex(ParentIdx) || Tree.Nodes[ParentIdx].NodeType != ESUDSParsedNodeType::Select)
		return;

	int TopSelectIdx = ParentIdx;
	// Find the node above the top of potentially nested select chain
	while (Tree.Nodes.IsValidIndex(ParentIdx) && Tree.Nodes[ParentIdx].NodeType == ESUDSParsedNodeType::Select)
	{
		TopSelectIdx = ParentIdx;
		ParentIdx = Tree.Nodes[ParentIdx].ParentNodeIdx;
	}

	if (Tree.Nodes.IsValidIndex(ParentIdx) &&  Tree.Nodes[ParentIdx].NodeType == ESUDSParsedNodeType::Choice)
	{
		// OK we found a choice above the select, this is OK
		return;
	}

	if (ParentIdx == -1)
	{
		// This means we hit the top of the chain without finding choice or select
		// We have to trust that FindLastChoiceNode will find it & it's valid (previous placement should resolve)

		// Connect top select to choice under last text
		if (Tree.Nodes.IsValidIndex(TopSelectIdx) && Tree.Nodes.IsValidIndex(Ctx.LastTextNodeIdx))
		{
			// Choice node might not be directly underneath
			const int ChoiceIdx = FindChoiceAfterTextNode(Tree, Ctx.LastTextNodeIdx);
			if (ChoiceIdx != -1)
			{
				auto& SelNode = Tree.Nodes[TopSelectIdx];
				auto& ChoiceNode = Tree.Nodes[ChoiceIdx];
				FSUDSParsedEdge NewEdge(LineNo);
				NewEdge.SourceNodeIdx = ChoiceIdx;
				NewEdge.TargetNodeIdx = TopSelectIdx;
				ChoiceNode.Edges.Add(NewEdge);
				SelNode.ParentNodeIdx = ChoiceIdx;
			}
		}
		return;
	}

	// If we got here, we navigated to the point above the select(s) and didn't end up on a root choice
	// So we need to insert one, after this index
	const int InsertIdx = ParentIdx + 1; // this happens to return 0 for having hit the start of the tree (-1), which is fine

	Tree.Nodes.Insert(FSUDSParsedNode(ESUDSParsedNodeType::Choice, IndentLevel, LineNo), InsertIdx);
	auto& NewChoice = Tree.Nodes[InsertIdx];
	auto& SelectNode = Tree.Nodes[InsertIdx + 1];
	// Add edge to the select, fixup the parent nodes for both
	NewChoice.Edges.Add(FSUDSParsedEdge(InsertIdx, InsertIdx + 1, LineNo));
	NewChoice.ParentNodeIdx = SelectNode.ParentNodeIdx;
	NewChoice.ChoicePath = SelectNode.ChoicePath;
	NewChoice.ConditionalPath = SelectNode.ConditionalPath;

	// Now for every other node after this, we have to fix up indexes that are >= InsertIdx
	// We don't fix up anything before, because we want things that pointed forward to the select to now point at the choice
	for (int i = InsertIdx + 1; i < Tree.Nodes.Num(); ++i)
	{
		if (i != InsertIdx)
		{
			auto& N = Tree.Nodes[i];
			if (N.ParentNodeIdx >= InsertIdx)
				++N.ParentNodeIdx;
			for (auto& E : N.Edges)
			{
				if (E.SourceNodeIdx >= InsertIdx)
					++E.SourceNodeIdx;
				if (E.TargetNodeIdx >= InsertIdx)
					++E.TargetNodeIdx;
			}
		}
	}

	// Also fix indent contexts
	for (auto& I : Tree.IndentLevelStack)
	{
		if (I.LastNodeIdx >= InsertIdx)
			++I.LastNodeIdx;
		if (I.LastTextNodeIdx >= InsertIdx)
			++I.LastTextNodeIdx;
	}
	// Also conditional blocks
	for (auto& CB : Tree.ConditionalBlocks)
	{
		if (CB.SelectNodeIdx >= InsertIdx)
			++CB.SelectNodeIdx;
	}
	// Fixup edge in progress
	if (Tree.EdgeInProgressNodeIdx >= InsertIdx)
		++Tree.EdgeInProgressNodeIdx;


	// Manually change the select node parent index afterwards (if we change it before it'll get adjusted again)
	SelectNode.ParentNodeIdx = InsertIdx;

	SetFallthroughForNewNode(Tree, NewChoice);
	
	
}

bool FSUDSScriptImporter::IsConditionalLine(const FStringView& Line)
{
	return Line.StartsWith(TEXT("[if")) ||
		Line.StartsWith(TEXT("[else")) ||
		Line.StartsWith(TEXT("[endif"));
}

bool FSUDSScriptImporter::ParseElseLine(const FStringView& Line,
                                        FSUDSScriptImporter::ParsedTree& Tree,
                                        int IndentLevel,
                                        int LineNo,
                                        const FString& NameForErrors,
                                        FSUDSMessageLogger* Logger,
                                        bool bSilent)
{
	if (Tree.ConditionalBlocks.IsValidIndex(Tree.CurrentConditionalBlockIdx))
	{
		// "else" changes the current "if" or "elseif" block to "else" & creates a new condition-free edge
		// Select or choice node should already be there
		// Select node may be turned into a choice later if choice is the first thing encountered

		auto& Block = Tree.ConditionalBlocks[Tree.CurrentConditionalBlockIdx];
		if (Block.Stage != EConditionalStage::ElseStage)
		{
			Block.Stage = EConditionalStage::ElseStage;
			Block.ConditionStr = "";
			const int NodeIdx = Block.SelectNodeIdx;
				
			auto& SelectNode = Tree.Nodes[NodeIdx];
			const int EdgeIdx = SelectNode.Edges.Add(FSUDSParsedEdge(NodeIdx, -1, LineNo));
			Tree.EdgeInProgressNodeIdx = NodeIdx;
			Tree.EdgeInProgressEdgeIdx = EdgeIdx;
			// Wind back the last node to select
			auto& Ctx = Tree.IndentLevelStack.Top();
			Ctx.LastNodeIdx = NodeIdx;

					
		}
		else
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: cannot have more than one 'else'"), *NameForErrors, LineNo);
		}
	}
	else
	{
		if (!bSilent)
			Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: 'else' with no matching 'if'"), *NameForErrors, LineNo);
	}
		
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ELSE  : %s"), LineNo, IndentLevel, *FString(Line));
	return true;
	
}

bool FSUDSScriptImporter::ParseEndIfLine(const FStringView& Line,
                                         FSUDSScriptImporter::ParsedTree& Tree,
                                         int IndentLevel,
                                         int LineNo,
                                         const FString& NameForErrors,
                                         FSUDSMessageLogger* Logger,
                                         bool bSilent)
{
	if (Tree.ConditionalBlocks.IsValidIndex(Tree.CurrentConditionalBlockIdx))
	{
		// Endif finishes the current block
		const auto& Block = Tree.ConditionalBlocks[Tree.CurrentConditionalBlockIdx];
		Tree.CurrentConditionalBlockIdx = Block.PreviousBlockIdx;
		// We must also clear the indent last node pointer, because we never want to auto-connect to conditionals
		// We'll let the final fallthrough pass connect things
		auto& Ctx = Tree.IndentLevelStack.Top();
		Ctx.LastNodeIdx = -1;
	}
	else
	{
		if (!bSilent)
			Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: 'endif' with no matching 'if'"), *NameForErrors, LineNo);
	}
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ENDIF : %s"), LineNo, IndentLevel, *FString(Line));
	return true;
	
}

bool FSUDSScriptImporter::ParseIfLine(const FStringView& Line,
                                            FSUDSScriptImporter::ParsedTree& Tree,
                                            const FString& ConditionStr,
                                            int IndentLevel,
                                            int LineNo,
                                            const FString& NameForErrors,
                                            FSUDSMessageLogger* Logger,
                                            bool bSilent)
{

	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: IF    : %s"), LineNo, IndentLevel, *FString(Line));

	// New if level always creates select node				
	const int NewNodeIdx = AppendNode(Tree, FSUDSParsedNode(ESUDSParsedNodeType::Select, IndentLevel, LineNo));
	auto& SelectNode = Tree.Nodes[NewNodeIdx];
	const int EdgeIdx = SelectNode.Edges.Add(FSUDSParsedEdge(NewNodeIdx, -1, LineNo));
	auto E = &SelectNode.Edges[EdgeIdx];
	{
		FString ParseError;
		if (!E->ConditionExpression.ParseFromString(ConditionStr, &ParseError))
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: %s"), *NameForErrors, LineNo, *ParseError);
		}
	}
	Tree.EdgeInProgressNodeIdx = NewNodeIdx;
	Tree.EdgeInProgressEdgeIdx = EdgeIdx;
			
	Tree.CurrentConditionalBlockIdx = Tree.ConditionalBlocks.Add(
		ConditionalContext(NewNodeIdx, Tree.CurrentConditionalBlockIdx, EConditionalStage::IfStage, ConditionStr));
	
	return true;
	
}

bool FSUDSScriptImporter::ParseElseIfLine(const FStringView& Line,
	ParsedTree& Tree,
	const FString& ConditionStr,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	if (!bSilent)
		UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: ELSEIF: %s"), LineNo, IndentLevel, *FString(Line));
	
	// "elseif" changes the current block state 
	// Select or choice node should already be there
	// Select node may be turned into a choice later if choice is the first thing encountered

	if (Tree.ConditionalBlocks.IsValidIndex(Tree.CurrentConditionalBlockIdx))
	{
		auto& Block = Tree.ConditionalBlocks[Tree.CurrentConditionalBlockIdx];
		if (Block.Stage != EConditionalStage::ElseStage)
		{
			Block.Stage = EConditionalStage::ElseIfStage;
			Block.ConditionStr = ConditionStr;
			const int NodeIdx = Block.SelectNodeIdx;
				
			auto& SelectOrChoiceNode = Tree.Nodes[NodeIdx];
			const int EdgeIdx = SelectOrChoiceNode.Edges.Add(FSUDSParsedEdge(NodeIdx, -1, LineNo));
			auto E = &SelectOrChoiceNode.Edges[EdgeIdx];
			{
				FString ParseError;
				if (!E->ConditionExpression.ParseFromString(ConditionStr, &ParseError))
				{
					if (!bSilent)
						Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: %s"), *NameForErrors, LineNo, *ParseError);
				}
			}
			Tree.EdgeInProgressNodeIdx = NodeIdx;
			Tree.EdgeInProgressEdgeIdx = EdgeIdx;
		}
		else
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: 'elseif' occurs after 'else'"), *NameForErrors, LineNo);
						
		}
					
	}
	else
	{
		if (!bSilent)
			Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: 'elseif' with no matching 'if'"), *NameForErrors, LineNo);
	}
	
	return true;
}

bool FSUDSScriptImporter::ParseConditionalLine(const FStringView& Line,
                                               FSUDSScriptImporter::ParsedTree& Tree,
                                               int IndentLevel,
                                               int LineNo,
                                               const FString& NameForErrors,
                                               FSUDSMessageLogger* Logger, 
                                               bool bSilent)
{

	if (!IsConditionalLine(Line))
		return false;
	
	if (Line.Equals(TEXT("[else]")))
	{
		return ParseElseLine(Line, Tree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		
	}
	else if (Line.Equals(TEXT("[endif]")))
	{
		return ParseEndIfLine(Line, Tree, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
	}
	else
	{
		const FString LineStr(Line);
		const FRegexPattern IfPattern(TEXT("^\\[if\\s+(.+)\\]$"));
		FRegexMatcher IfRegex(IfPattern, LineStr);
		if (IfRegex.FindNext())
		{
			const FString ConditionStr = IfRegex.GetCaptureGroup(1);
			return ParseIfLine(Line, Tree, ConditionStr, IndentLevel, LineNo, NameForErrors, Logger, bSilent);
		}
		else
		{
			const FRegexPattern ElseIfPattern(TEXT("^\\[elseif\\s+(.+)\\]$"));
			FRegexMatcher ElseIfRegex(ElseIfPattern, LineStr);
			if (ElseIfRegex.FindNext())
			{
				const FString ConditionStr = ElseIfRegex.GetCaptureGroup(1);
				return ParseElseIfLine(Line, Tree, ConditionStr, IndentLevel, LineNo, NameForErrors, Logger, bSilent);

			}
		}
	}
		
	return false;
}

bool FSUDSScriptImporter::ParseGotoLabelLine(const FStringView& Line,
                                             FSUDSScriptImporter::ParsedTree& Tree,
                                             int IndentLevel,
                                             int LineNo,
                                             const FString& NameForErrors,
                                             FSUDSMessageLogger* Logger, 
                                             bool bSilent)
{
	// We've already established that line starts with ':'
	// There should not be any spaces in the label
	const FString LineStr(Line);
	const FRegexPattern LabelPattern(TEXT("^\\:\\s*(\\w+)$"));
	FRegexMatcher LabelRegex(LabelPattern, LineStr);
	if (LabelRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: LABEL : %s"), LineNo, IndentLevel, *FString(Line));
		// lowercase goto labels so case insensitive
		FString Label = LabelRegex.GetCaptureGroup(1).ToLower();
		if (Label == EndGotoLabel)
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: Label 'end' is reserved and cannot be used in the script, ignoring"), *NameForErrors, LineNo);
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
			Logger->Logf(ELogVerbosity::Warning, TEXT("Error in %s line %d: Badly formed goto label"), *NameForErrors, LineNo);
	}
	// Always return true to carry on, may not be used
	return true;
}

bool FSUDSScriptImporter::ParseGotoLine(const FStringView& Line,
                                        FSUDSScriptImporter::ParsedTree& Tree,
                                        int IndentLevel,
                                        int LineNo,
                                        const FString& NameForErrors,
                                        FSUDSMessageLogger* Logger, 
                                        bool bSilent)
{
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	// Allow both 'goto' and 'go to'
	const FRegexPattern GotoPattern(TEXT("^\\[go[ ]?to\\s+(\\w+)\\s*\\]$"));
	FRegexMatcher GotoRegex(GotoPattern, LineStr);
	if (GotoRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: GOTO  : %s"), LineNo, IndentLevel, *FString(Line));
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
	return false;
}

bool FSUDSScriptImporter::ParseGosubLine(const FStringView& InLine,
	ParsedTree& Tree,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	// Attempt to find existing ID
	// We need these in order to save the return stack
	FString GosubID;
	FStringView Line = InLine;
	// If this is a continuation line, we shouldn't generate one, but we need to trim it off if it's there
	bool bFoundID = RetrieveAndRemoveGosubID(Line, GosubID);
	
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	// Allow both 'gosub' and 'go sub'
	const FRegexPattern GosubPattern(TEXT("^\\[go[ ]?sub\\s+(\\w+)\\s*\\]$"));
	FRegexMatcher GosubRegex(GosubPattern, LineStr);
	if (GosubRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: GOSUB  : %s"), LineNo, IndentLevel, *FString(Line));
		// lower case label so case insensitive
		const FString Label = GosubRegex.GetCaptureGroup(1).ToLower();

		// You CANNOT "gosub end"
		if (Label == EndGotoLabel)
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: You cannot 'gosub end', will never return. Did you mean goto?"), *NameForErrors, LineNo);
			
		}
		else
		{
			// note that we do NOT try to resolve the label here, to allow forward jumps.
			const auto& Ctx = Tree.IndentLevelStack.Top();
			// A gosub will become a node of its own in the final runtime
			// Therefore we don't need to alias labels like we do with gotos
			AppendNode(Tree, FSUDSParsedNode(Label, GosubID, IndentLevel, LineNo));
		}
		return true;
	}
	return false;
	
}

bool FSUDSScriptImporter::ParseReturnLine(const FStringView& Line,
	ParsedTree& Tree,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	const FRegexPattern ReturnPattern(TEXT("^\\[return\\s*\\]$"));
	FRegexMatcher ReturnRegex(ReturnPattern, LineStr);
	if (ReturnRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: RETURN  : %s"), LineNo, IndentLevel, *FString(Line));
		AppendNode(Tree, FSUDSParsedNode(ESUDSParsedNodeType::Return, IndentLevel, LineNo));
		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseSetLine(const FStringView& InLine,
                                       FSUDSScriptImporter::ParsedTree& Tree,
                                       int IndentLevel,
                                       int LineNo,
                                       const FString& NameForErrors,
                                       FSUDSMessageLogger* Logger,
                                       bool bSilent)
{
	// Attempt to find existing text ID, for string literals
	// For multiple lines, may not be present until last line (but in fact can be on any line)
	// We generate anyway, because it can be overriden by later lines, but makes sure we have one always
	FString TextID;
	FStringView Line = InLine;
	// TextID may be blank after this, that's OK - we fix at the end once we know what IDs are used
	RetrieveAndRemoveTextID(Line, TextID);
	
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	// Accept forms:
	// [set Var Expression]
	// [set Var = Expression] (more readable in the case of non-trivial expressions)
	const FRegexPattern SetPattern(TEXT("^\\[set\\s+(\\S+)\\s+(?:=\\s+)?([^\\]]+)\\]$"));
	FRegexMatcher SetRegex(SetPattern, LineStr);
	if (SetRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: SET   : %s"), LineNo, IndentLevel, *FString(Line));

		FString Name = SetRegex.GetCaptureGroup(1);
		FString ExprStr = SetRegex.GetCaptureGroup(2).TrimStartAndEnd(); // trim because capture accepts spaces in quotes

		FSUDSExpression Expr;
		{
			FString ParseError;
			if (Expr.ParseFromString(ExprStr, &ParseError))
			{
				if (Expr.IsTextLiteral())
				{
					AppendNode(Tree, FSUDSParsedNode(Name, Expr, TextID, IndentLevel, LineNo));
				}
				else
				{
					AppendNode(Tree, FSUDSParsedNode(Name, Expr, IndentLevel, LineNo));
				}
				return true;
			}
			else
			{
				if (!bSilent)
					Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: %s"), *NameForErrors, LineNo, *ParseError);
			}
		}
	}
	return false;
}

bool FSUDSScriptImporter::ParseImportSettingLine(const FStringView& Line,
	ParsedTree& Tree,
	int IndentLevel,
	int LineNo,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	// Unfortunately FRegexMatcher doesn't support FStringView
	const FString LineStr(Line);
	const FRegexPattern ImportSetPattern(TEXT("^\\[importsetting\\s+(\\S+)\\s+(?:=\\s+)?([^\\]]+)\\]$"));
	FRegexMatcher ImportSetRegex(ImportSetPattern, LineStr);
	if (ImportSetRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: IMPORTSETTING: %s"), LineNo, IndentLevel, *FString(Line));

		const FString Name = ImportSetRegex.GetCaptureGroup(1);
		const FString ExprStr = ImportSetRegex.GetCaptureGroup(2).TrimStartAndEnd(); // trim because capture accepts spaces in quotes
		
		FSUDSExpression Expr;
		{
			FString ParseError;
			if (Expr.ParseFromString(ExprStr, &ParseError))
			{
				if (Expr.IsLiteral())
				{
					// Import settings affect importer state directly, no nodes
					if (Name.Compare("GenerateSpeakerLinesFromChoices", ESearchCase::IgnoreCase) == 0)
					{
						if (Expr.GetLiteralValue().GetType() == ESUDSValueType::Boolean)
						{
							bOverrideGenerateSpeakerLineForChoice = Expr.GetBooleanLiteralValue();
						}
						else
						{
							if (!bSilent)
								Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: [importsetting GenerateSpeakerLinesFromChoices ...] requires a boolean literal"), *NameForErrors, LineNo);
						}
					}
					else if (Name.Compare("SpeakerIDForGeneratedLinesFromChoices", ESearchCase::IgnoreCase) == 0)
					{
						if (Expr.GetLiteralValue().GetType() == ESUDSValueType::Name)
						{
							// Speaker IDs are strings, but we go via FName to avoid translation
							OverrideChoiceSpeakerID = Expr.GetNameLiteralValue().ToString();
						}
						else
						{
							if (!bSilent)
								Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: [importsetting SpeakerIDForGeneratedLinesFromChoices ...] requires a Name literal e.g. (`Value`)"), *NameForErrors, LineNo);
						}
					}
				
					return true;
				}
				else
				{
					if (!bSilent)
						Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: importsetting only accepts literal values"), *NameForErrors, LineNo);
				}
			}
			else
			{
				if (!bSilent)
					Logger->Logf(ELogVerbosity::Error, TEXT("Error in %s line %d: %s"), *NameForErrors, LineNo, *ParseError);
			}
		}
	}
	return false;
	
}

bool FSUDSScriptImporter::ParseEventLine(const FStringView& Line,
                                         FSUDSScriptImporter::ParsedTree& Tree,
                                         int IndentLevel,
                                         int LineNo,
                                         const FString& NameForErrors,
                                         FSUDSMessageLogger* Logger,
                                         bool bSilent)
{
	const FString LineStr(Line);
	const FRegexPattern EventPattern(TEXT("^\\[event\\s+(\\w+)([^\\]]*)\\]$"));
	FRegexMatcher EventRegex(EventPattern, LineStr);
	if (EventRegex.FindNext())
	{
		if (!bSilent)
			UE_LOG(LogSUDSImporter, VeryVerbose, TEXT("%3d:%2d: EVENT : %s"), LineNo, IndentLevel, *FString(Line));

		FSUDSParsedNode Node(ESUDSParsedNodeType::Event, IndentLevel, LineNo);
		
		Node.Identifier = EventRegex.GetCaptureGroup(1);

		if (EventRegex.GetCaptureGroupBeginning(2) != INDEX_NONE)
		{
			// Has arguments, all lumped together
			// Capture using a sub-regex which can detect quoted strings, split by commas			
			FString AllArgs = EventRegex.GetCaptureGroup(2).TrimStartAndEnd();
			const FRegexPattern ArgPattern(TEXT("((\\\"[^\\\"]*\\\"|[^,\\\"]+))"));
			FRegexMatcher ArgRegex(ArgPattern, AllArgs);
			while (ArgRegex.FindNext())
			{
				// then process the quote
				FString ArgStr = ArgRegex.GetCaptureGroup(1).TrimStartAndEnd();
				if (ArgStr.Len() == 0)
					continue;
				
				FSUDSExpression Expr;
				FString ParseError;
				if (Expr.ParseFromString(ArgStr, &ParseError))
				{
					// note: no localisation of event literals, they're just strings
					// we assume the receiver of the event will set localised text to variables if they want
					Node.EventArgs.Add(Expr);
				}
				else
				{
					if (!bSilent)
						Logger->Logf(ELogVerbosity::Warning, TEXT("Error in %s line %d: Literal argument %d ('%s') invalid: %s"), *NameForErrors, LineNo, Node.EventArgs.Num() + 1, *ArgStr, *ParseError);
				}
			}
		}

		AppendNode(Tree, Node);

		return true;
	}
	return false;
}

bool FSUDSScriptImporter::ParseTextLine(const FStringView& InLine,
                                        FSUDSScriptImporter::ParsedTree& Tree,
                                        int IndentLevel,
                                        int LineNo,
                                        const FString& NameForErrors,
                                        FSUDSMessageLogger* Logger,
                                        bool bSilent)
{
	auto& Ctx = Tree.IndentLevelStack.Top();

	// Attempt to find existing text ID
	// For multiple lines, may not be present until last line (but in fact can be on any line)
	FString TextID;
	FStringView Line = InLine;
	// Retrieve, but don't generate text ID at this point
	// If this is a continuation line, we shouldn't generate one, but we need to trim it off if it's there
	// TextID may be blank after this, that's OK - we fix at the end once we know what IDs are used
	RetrieveAndRemoveTextID(Line, TextID);
	
	const FString LineStr(Line);
	const FRegexPattern SpeakerPattern(TEXT("^(\\S+)\\:\\s*(.+)$"));
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
		Ctx.LastTextNodeIdx = AppendNode(Tree, FSUDSParsedNode(Speaker, Text, TextID, GetTextMetadataForNextEntry(IndentLevel), IndentLevel, LineNo));

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
		}
		else
		{
			if (!bSilent)
				Logger->Logf(ELogVerbosity::Warning, TEXT("Error in %s line %d: Text newline continuation is not immediately after a speaker line. Ignoring."), *NameForErrors, LineNo);
			// We still return true to allow continue	
		}
	}
	
	return true;


}

bool FSUDSScriptImporter::RetrieveAndRemoveTextID(FStringView& InOutLine, FString& OutTextID)
{

	FString LineWithout;
	int Number;
	if (RetrieveTextIDFromLine(InOutLine, OutTextID, Number))
	{
		TextIDHighestNumber = Number;
		return true;
	}

	return false;

}

bool FSUDSScriptImporter::RetrieveTextIDFromLine(FStringView& InOutLine, FString& OutTextID, int& OutNumber)
{
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
		OutNumber = FCString::Strtoi(*TextIDRegex.GetCaptureGroup(2), nullptr, 16);
		return true;
	}

	return false;
	
}

bool FSUDSScriptImporter::RetrieveAndRemoveGosubID(FStringView& InOutLine, FString& OutTextID)
{

	FString LineWithout;
	int Number;
	if (RetrieveGosubIDFromLine(InOutLine, OutTextID, Number))
	{
		GosubIDHighestNumber = Number;
		return true;
	}

	return false;

}

bool FSUDSScriptImporter::RetrieveGosubIDFromLine(FStringView& InOutLine, FString& OutID, int& OutNumber)
{
	const FString LineStr(InOutLine);
	const FRegexPattern IDPattern(TEXT("(\\@GS([0-9a-fA-F]+)\\@)"));
	FRegexMatcher IDRegex(IDPattern, LineStr);
	if (IDRegex.FindNext())
	{
		OutID = IDRegex.GetCaptureGroup(1);
		// Chop the incoming string to the left of the TextID
		InOutLine = InOutLine.Left(IDRegex.GetCaptureGroupBeginning(1));
		// Also trim right
		InOutLine = InOutLine.TrimEnd();
		// FDefaultValueHelper::ParseInt requires an "0x" prefix but we're not using that
		// Plus does extra checking we don't need
		OutNumber = FCString::Strtoi(*IDRegex.GetCaptureGroup(2), nullptr, 16);
		return true;
	}

	return false;
	
}

FString FSUDSScriptImporter::GenerateTextID()
{
	// Generate a new text ID just based on ascending numbers
	// We don't actually base this on the line but we have it for future possible use
	// Since it's a string, format exactly as in the sud file 
	return FString::Printf(TEXT("@%04x@"), ++TextIDHighestNumber);
}

FString FSUDSScriptImporter::GetCurrentTreePath(const FSUDSScriptImporter::ParsedTree& Tree)
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

FString FSUDSScriptImporter::GetCurrentTreeConditionalPath(const FSUDSScriptImporter::ParsedTree& Tree)
{
	// Like GetCurrentTreePath, but for conditional blocks
	// Cannot fall through to blocks that aren't on the same conditional path
	FStringBuilderBase B;
	int BlockIdx = Tree.CurrentConditionalBlockIdx;
	B.Append("/");
	// work backwards, hence prepend
	while (BlockIdx != -1)
	{
		const FString& ConditionStr = Tree.ConditionalBlocks[BlockIdx].ConditionStr;
		// Note: add the "/" even if ConditionStr is empty, because it means it's an else level
		// Not including it can cause an if block to fall through to its own else
		B.Prepend(FString::Printf(TEXT("%s%s"), *TreePathSeparator, *ConditionStr));
		BlockIdx = Tree.ConditionalBlocks[BlockIdx].PreviousBlockIdx;
	}
	return B.ToString();
	
}

void FSUDSScriptImporter::SetFallthroughForNewNode(FSUDSScriptImporter::ParsedTree& Tree, FSUDSParsedNode& NewNode)
{
	// Choice nodes are allowed to be falled through to now
	if (NewNode.NodeType == ESUDSParsedNodeType::Choice)
	{

		// But, disable fallthrough for any parent select nodes all the way up the chain
		int PrevIdx = NewNode.ParentNodeIdx;
		while (Tree.Nodes.IsValidIndex(PrevIdx) && Tree.Nodes[PrevIdx].NodeType == ESUDSParsedNodeType::Select)
		{
			Tree.Nodes[PrevIdx].AllowFallthrough = false;
			PrevIdx = Tree.Nodes[PrevIdx].ParentNodeIdx; 
		}
	}
	else
	{
		if (Tree.Nodes.IsValidIndex(NewNode.ParentNodeIdx))
		{
			auto& PrevNode = Tree.Nodes[NewNode.ParentNodeIdx];
			// Do not allow fallthrough to this node for selects that are themselves children of other selects or choices
			// It means they're compound conditionals / choices and we should only fall through to the root of that
			if (NewNode.NodeType == ESUDSParsedNodeType::Select &&
				(PrevNode.NodeType == ESUDSParsedNodeType::Choice || PrevNode.NodeType == ESUDSParsedNodeType::Select))
			{
				NewNode.AllowFallthrough = false;
			}
		}
	}
	
}

int FSUDSScriptImporter::AppendNode(FSUDSScriptImporter::ParsedTree& Tree, const FSUDSParsedNode& InNode)
{
	auto& Ctx = Tree.IndentLevelStack.Top();

	const int NewIndex = Tree.Nodes.Add(InNode);

	// Set the tree path of the node (post-add)
	auto& NewNode = Tree.Nodes[NewIndex];
	NewNode.ChoicePath = GetCurrentTreePath(Tree);
	NewNode.ConditionalPath = GetCurrentTreeConditionalPath(Tree);

	// Use pending edge if present; that could be because this is under a choice node, or a condition
	if (auto E = GetEdgeInProgress(Tree))
	{
		E->TargetNodeIdx = NewIndex;

		const int PrevNodeIdx = E->SourceNodeIdx;
		NewNode.ParentNodeIdx = PrevNodeIdx;
		SetFallthroughForNewNode(Tree, NewNode);
		Tree.EdgeInProgressNodeIdx = -1;
		Tree.EdgeInProgressEdgeIdx = -1;
	}
	else
	{
		const int PrevNodeIdx = Ctx.LastNodeIdx;
		if (Tree.Nodes.IsValidIndex(PrevNodeIdx))
		{
			// Append this node onto the last one
			auto& PrevNode = Tree.Nodes[PrevNodeIdx];
			// Auto-connect new nodes to previous nodes
			// Valid for nodes with only one output node
			// A new node with no pending edge following any other type may be connected via fallthrough at
			// the end of parsing
			// Don't allow connection of fallthrough nodes to choices, unless they're to another select node 
			if (PrevNode.NodeType != ESUDSParsedNodeType::Choice ||
			 	(NewNode.NodeType == ESUDSParsedNodeType::Select))
			{
				PrevNode.Edges.Add(FSUDSParsedEdge(PrevNodeIdx, NewIndex, InNode.SourceLineNo));
				NewNode.ParentNodeIdx = PrevNodeIdx;
			}

			// Don't throw an error otherwise, because prev index can be a choice due to fallthrough
			// This will be connected up at the end

		}

		SetFallthroughForNewNode(Tree, NewNode);

	}

	// All goto labels in scope at this point now point to this node
	for (auto Label : Tree.PendingGotoLabels)
	{
		Tree.GotoLabelList.Add(Label, NewIndex);
	}
	Tree.PendingGotoLabels.Reset();

	Ctx.LastNodeIdx = NewIndex;
	Ctx.ThresholdIndent = FMath::Min(Ctx.ThresholdIndent, InNode.OriginalIndent);
	

	return NewIndex;
}

bool FSUDSScriptImporter::SelectNodeIsMissingElsePath(const FSUDSScriptImporter::ParsedTree& Tree, const FSUDSParsedNode& Node)
{
	for (auto& E : Node.Edges)
	{
		if (E.ConditionExpression.IsEmpty())
		{
			// This is an else
			return false;
		}
	}

	// If the first node the select points to (other than a nested select) is a choice, we never add an else
	// note: you should never have an if/else where one branch has a set/text node and the other has a choice node, this is badly formed
	// perhaps we should validate that
	if (Node.Edges.Num() > 0)
	{
		int NextIdx = Node.Edges[0].TargetNodeIdx;
		// While so that we can follow nested selects to first resolved
		while (Tree.Nodes.IsValidIndex(NextIdx))
		{
			auto N = Tree.Nodes[NextIdx];
			if (N.NodeType == ESUDSParsedNodeType::Select)
			{
				// Nested select, cascade down
				if (N.Edges.Num() > 0)
				{
					NextIdx = N.Edges[0].TargetNodeIdx;
				}
				else
				{
					NextIdx = -1;
				}
			}
			else
			{
				if (N.NodeType == ESUDSParsedNodeType::Choice)
				{
					// We never add fallthrough else paths for selection between choices
					return false;
				}
				NextIdx = -1;
			}
		}
	}
	
	return true;
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

void FSUDSScriptImporter::ConnectRemainingNodes(FSUDSScriptImporter::ParsedTree& Tree, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent)
{
	// Now we go through all nodes, resolving gotos, and finding links that don't go anywhere & making
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
		// We check for dead-end nodes, and for select nodes with no "else" (in case all conditions fail)
		const bool bIsSelectNodeMissingElse = Node.NodeType == ESUDSParsedNodeType::Select && SelectNodeIsMissingElsePath(Tree, Node);
		if (Node.Edges.IsEmpty() || bIsSelectNodeMissingElse)
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
							Logger->Logf(ELogVerbosity::Warning, TEXT("Error in %s line %d: Goto label '%s' was not found, references to it will goto End"), *NameForErrors, Node.SourceLineNo, *Node.Identifier);
					}
				}
			}
			// Return nodes never fall through, they're always going back
			// Gosubs DO come through here though, fallthrough is AFTER the return
			else if (Node.NodeType != ESUDSParsedNodeType::Return) 
			{
				// Find the next node which is at a higher indent level than this
				// For a select node missing else, treat the indent as 1 inward, since it's really falling through from a nested part of the select
				const int IndentLessThan = bIsSelectNodeMissingElse ? Node.OriginalIndent + 1 : Node.OriginalIndent;
				const auto FallthroughIdx = FindFallthroughNodeIndex(Tree, i+1, Node.ChoicePath, Node.ConditionalPath);
				if (Tree.Nodes.IsValidIndex(FallthroughIdx))
				{
					Node.Edges.Add(FSUDSParsedEdge(i, FallthroughIdx, Node.SourceLineNo));
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
					const auto FallthroughIdx = FindFallthroughNodeIndex(Tree, i+1, Node.ChoicePath, Node.ConditionalPath);
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

void FSUDSScriptImporter::GenerateTextIDs(ParsedTree& Tree)
{
	// This is where we generate any missing TextIDs for Text nodes, Set nodes (optionally) and Choice edges
	// We don't want to do it as we go along, because if a script has some lines with TextIDs and new inserted lines
	// in between, we won't know from a top-down scan which IDs have already been used. Later explicit TextIDs in
	// the script could cause an ID clash with generated ones for inserted lines

	for (auto& Node : Tree.Nodes)
	{
		switch (Node.NodeType)
		{
		case ESUDSParsedNodeType::Text:
			if (Node.TextID.IsEmpty())
			{
				Node.TextID = GenerateTextID();
			}
			break;
		case ESUDSParsedNodeType::Choice:
			// Text for choices is on edges
			{
				for (auto& Edge : Node.Edges)
				{
					if (!Edge.Text.IsEmpty() && Edge.TextID.IsEmpty())
					{
						Edge.TextID = GenerateTextID();
					}

					// If this choice generated a speaker line, we need to give that the same text ID
					// It will always be directly after the choice edge
					if (Tree.Nodes.IsValidIndex(Edge.TargetNodeIdx))
					{
						auto& NextNode = Tree.Nodes[Edge.TargetNodeIdx];
						if (NextNode.NodeType == ESUDSParsedNodeType::Text && NextNode.TextID.IsEmpty() &&
							NextNode.Text == Edge.Text)
						{
							NextNode.TextID = Edge.TextID;
						}
					}
				}
			}
			break;
		case ESUDSParsedNodeType::SetVariable:
			if (Node.Expression.IsTextLiteral())
			{
				// Text must be localised
				if (Node.TextID.IsEmpty())
				{
					Node.TextID = GenerateTextID();
				}
			}
			break;
		default:
			break;
		}

	}
	
}

bool FSUDSScriptImporter::PostImportSanityCheck(const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent)
{
	bool bOK = true;
	for (int i = 0; i < BodyTree.Nodes.Num(); ++i)
	{
		auto& Node = BodyTree.Nodes[i];

		if (Node.NodeType == ESUDSParsedNodeType::Choice)
		{
			// Check all of them so we can report all errors, rather than early-out
			bOK = ChoiceNodeCheckPaths(Node, NameForErrors, Logger, bSilent) && bOK;
		}
	}
	return bOK;
}

bool FSUDSScriptImporter::ChoiceNodeCheckPaths(const FSUDSParsedNode& ChoiceNode,
                                               const FString& NameForErrors,
                                               FSUDSMessageLogger* Logger,
                                               bool bSilent)
{
	return RecurseChoiceNodeCheckPaths(ChoiceNode, ChoiceNode, NameForErrors, Logger, bSilent);
}

bool FSUDSScriptImporter::RecurseChoiceNodeCheckPaths(const FSUDSParsedNode& OrigChoiceNode,
	const FSUDSParsedNode& CurrChoiceNode,
	const FString& NameForErrors,
	FSUDSMessageLogger* Logger,
	bool bSilent)
{
	bool bOK = true;
	for (const auto& Edge: CurrChoiceNode.Edges)
	{
		// We want to make sure that every choice path leads to a speaker line, before it leads to another choice
		// A choice that leads directly to another choice can't be properly represented in dialogue; choices have to
		// be anchored by speaker lines so proceeding to another choice directly after a choice is made is wrong
		// Usually this will be caused by a bad goto but could also be just bad nesting
		// Every path has to lead to a speaker node before a choice
		bOK = RecurseChoiceNodeCheckPaths(OrigChoiceNode, Edge, NameForErrors, Logger, bSilent) && bOK;
	}
	return bOK;
}

bool FSUDSScriptImporter::RecurseChoiceNodeCheckPaths(const FSUDSParsedNode& ChoiceNode, const FSUDSParsedEdge& Edge,
                                                      const FString& NameForErrors,
                                                      FSUDSMessageLogger* Logger,
                                                      bool bSilent)
{
	const FSUDSParsedNode* TargetNode = GetNode(Edge.TargetNodeIdx);
	while (TargetNode)
	{
		switch (TargetNode->NodeType)
		{
		case ESUDSParsedNodeType::Text:
			// We're OK, found a speaker line
			return true;
		case ESUDSParsedNodeType::Choice:
			// Definitely not ok, we found a choice node before a speaker node
			Logger->Logf(ELogVerbosity::Error,
			             TEXT(
				             "%s: Choice '%s' on line %d needs a speaker line between it and the next choice at line %d. Choices MUST show another speaker line before the next choice."),
			             *NameForErrors,
			             *Edge.Text,
			             Edge.SourceLineNo,
			             TargetNode->SourceLineNo);
			return false;
		case ESUDSParsedNodeType::Select:
			{
				// Recurse selects; but in this case we can have nested choices underneath
				bool bOK = true;
				for (const auto& SelEdge : TargetNode->Edges)
				{
					const auto SelTarget = GetNode(SelEdge.TargetNodeIdx);
					if (SelTarget)
					{
						if (SelTarget->NodeType == ESUDSParsedNodeType::Choice)
						{
							// First level of nested choices is OK; they will be combined with the original choice
							// We don't need to recurse here since this choice will itself
						}
						else
						{
							bOK = RecurseChoiceNodeCheckPaths(ChoiceNode, SelEdge, NameForErrors, Logger, bSilent);
						}
					}
				}
				return bOK;
			}
		case ESUDSParsedNodeType::SetVariable:
		case ESUDSParsedNodeType::Event:
			// Continue (linear)
			if (TargetNode->Edges.Num() > 0)
			{
				TargetNode = GetNode(TargetNode->Edges[0].TargetNodeIdx);
			}
			else
			{
				// This can happen if the event is the last line, but also nested in a choice
				return true;
			}
			break;
		case ESUDSParsedNodeType::Gosub:
		case ESUDSParsedNodeType::Return:
			// We can't really check the gosub/return statically, this will be a runtime error
			return true;
		case ESUDSParsedNodeType::Goto:
			{
				// Follow the goto (if end, will result in null)
				const int GotoIdx = GetGotoTargetNodeIndex(BodyTree, TargetNode->Identifier);
				TargetNode = GetNode(GotoIdx);
				break;
			}
		}
	}

	return true;
}

int FSUDSScriptImporter::FindFallthroughNodeIndex(FSUDSScriptImporter::ParsedTree& Tree,
                                                  int StartNodeIndex,
                                                  const FString& FromChoicePath,
                                                  const FString& FromConditionalPath)
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
		// We used to require that N.OriginalIndent < IndentLessThan here
		// However, this is actually not needed, since indentation only controls association with choice paths, otherwise
		// it's irrelevant. And we already check that things only fall through if they're on the same choice/conditional
		// path (or a superset of it). 
		if (N.AllowFallthrough &&
			FromChoicePath.StartsWith(N.ChoicePath) &&
			FromConditionalPath.StartsWith(N.ConditionalPath))
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

FMD5Hash FSUDSScriptImporter::CalculateHash(const TCHAR* Buffer, int32 Len)
{
	FMD5Hash Hash;
	FMD5 MD5;

	MD5.Update((uint8*)Buffer, Len * sizeof(TCHAR));

	Hash.Set(MD5);
	return Hash;
	
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
						// Always include speaker metadata
						StringTable->GetMutableStringTable()->SetMetaData(InNode.TextID, FName("Speaker"), InNode.Identifier);
						// Other metadata
						for (auto Pair : InNode.TextMetadata)
						{
							StringTable->GetMutableStringTable()->SetMetaData(InNode.TextID, Pair.Key, Pair.Value);	
						}
						
						auto TextNode = NewObject<USUDSScriptNodeText>(Asset);
						TextNode->Init(InNode.Identifier, FText::FromStringTable (StringTable->GetStringTableId(), InNode.TextID), InNode.SourceLineNo);
						Node = TextNode;
						break;
					}
				case ESUDSParsedNodeType::Choice:
					{
						auto ChoiceNode = NewObject<USUDSScriptNode>(Asset);
						ChoiceNode->InitChoice(InNode.SourceLineNo);
						Node = ChoiceNode;
						break;
					}
				case ESUDSParsedNodeType::Select:
					{
						auto SelectNode = NewObject<USUDSScriptNode>(Asset);
						SelectNode->InitSelect(InNode.SourceLineNo);
						Node = SelectNode;
						break;
					}
				case ESUDSParsedNodeType::SetVariable:
					{
						auto SetNode = NewObject<USUDSScriptNodeSet>(Asset);
						// For text literals, re-point to string table
						FSUDSExpression Expr = InNode.Expression;
						if (Expr.IsTextLiteral())
						{
							StringTable->GetMutableStringTable()->SetSourceString(InNode.TextID, Expr.GetTextLiteralValue().ToString());
							Expr.SetTextLiteralValue(FText::FromStringTable (StringTable->GetStringTableId(), InNode.TextID));
						}
						SetNode->Init(InNode.Identifier, Expr, InNode.SourceLineNo);
						Node = SetNode;
						break;
					}
				case ESUDSParsedNodeType::Event:
					{
						auto EvtNode = NewObject<USUDSScriptNodeEvent>(Asset);
						EvtNode->Init(InNode.Identifier, InNode.EventArgs, InNode.SourceLineNo);
						Node = EvtNode;
						break;
					}
				case ESUDSParsedNodeType::Gosub:
					{
						// Validate gosub label at this point
						auto GosubNode = NewObject<USUDSScriptNodeGosub>(Asset);
						GosubNode->Init(InNode.Identifier, InNode.TextID, InNode.SourceLineNo);
						Node = GosubNode;
						break;
					}
				case ESUDSParsedNodeType::Return:
					{
						auto ReturnNode = NewObject<USUDSScriptNode>(Asset);
						ReturnNode->InitReturn(InNode.SourceLineNo);
						Node = ReturnNode;
						break;
					}
				case ESUDSParsedNodeType::Goto:
					// Gotos do not become nodes, just fixed edges
				default: ;
					break;
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
					Node->AddEdge(FSUDSScriptEdge(nullptr, ESUDSEdgeType::Continue, InNode.SourceLineNo));
				}
				else
				{
					for (auto& InEdge : InNode.Edges)
					{
						ESUDSEdgeType NewEdgeType;

						const FSUDSParsedNode *InTargetNode = GetNode(Tree, InEdge.TargetNodeIdx);				

						switch (InNode.NodeType)
						{
						case ESUDSParsedNodeType::Text:
							if (InTargetNode && InTargetNode->NodeType == ESUDSParsedNodeType::Choice)
							{
								// Text -> Choice is chained
								NewEdgeType = ESUDSEdgeType::Chained;
							}
							else
							{
								NewEdgeType = ESUDSEdgeType::Continue;
							}
							break;
						case ESUDSParsedNodeType::Choice:
							if (InEdge.Text.IsEmpty() && InTargetNode && InTargetNode->NodeType == ESUDSParsedNodeType::Select)
							{
								// Choice->Select with no text is chained
								NewEdgeType = ESUDSEdgeType::Chained;
							}
							else
							{
								NewEdgeType = ESUDSEdgeType::Decision;
							}
							break;
						case ESUDSParsedNodeType::Select:
							// All edges under selects are conditions
							NewEdgeType = ESUDSEdgeType::Condition;
							break;
						default:
						case ESUDSParsedNodeType::SetVariable:
						case ESUDSParsedNodeType::Goto:
						case ESUDSParsedNodeType::Event:
							NewEdgeType = ESUDSEdgeType::Continue;
							break;
						};

						USUDSScriptNode* TargetNode = nullptr;
						
						if (InTargetNode)
						{
							
							if (InTargetNode->NodeType == ESUDSParsedNodeType::Goto)
							{
								// Resolve GOTOs immediately, point them directly at node goto points to
								int Idx = GetGotoTargetNodeIndex(Tree, InTargetNode->Identifier);
								// -1 means "Goto end", leave target null in that case
								if (Idx != -1)
								{
									const int NewTargetIndex = IndexRemap[Idx];
									TargetNode = (*pOutNodes)[NewTargetIndex];
								}
							}
							else
							{
								const int NewTargetIndex = IndexRemap[InEdge.TargetNodeIdx];
								TargetNode = (*pOutNodes)[NewTargetIndex];
							}

						}

						FSUDSScriptEdge NewEdge(TargetNode, NewEdgeType, InEdge.SourceLineNo);
						NewEdge.SetCondition(InEdge.ConditionExpression);
						NewEdge.SetTargetNode(TargetNode);

						if (!InEdge.TextID.IsEmpty() && !InEdge.Text.IsEmpty())
						{
							StringTable->GetMutableStringTable()->SetSourceString(InEdge.TextID, InEdge.Text);
							NewEdge.SetText(FText::FromStringTable(StringTable->GetStringTableId(), InEdge.TextID));
							// Always include speaker metadata, always the player in a choice
							// Identify that it's a choice so translators know that there may be more limited space
							StringTable->GetMutableStringTable()->SetMetaData(InEdge.TextID, FName("Speaker"), "Player (Choice)");
							// Other metadata
							for (auto Pair : InEdge.TextMetadata)
							{
								StringTable->GetMutableStringTable()->SetMetaData(InEdge.TextID, Pair.Key, Pair.Value);	
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

