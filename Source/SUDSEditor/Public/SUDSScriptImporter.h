#pragma once

#include "CoreMinimal.h"

class USUDSScript;
DECLARE_LOG_CATEGORY_EXTERN(LogSUDSImporter, Verbose, All);

struct SUDSEDITOR_API FSUDSParsedEdge
{
public:
	/// Text associated with this edge (if a player choice option)
	FString Text;
	/// Identifier of the text, for the string table
	FString TextID;
	/// The line this edge was created on
	int SourceLineNo;

	// TODO: Conditions

	int TargetNodeIdx = -1;

	FSUDSParsedEdge(int LineNo) : SourceLineNo(LineNo){}

	FSUDSParsedEdge(int ToNodeIdx, int LineNo, const FString& InText = "", const FString& InTextID = "")
		: Text(InText),
		  TextID(InTextID),
		  SourceLineNo(LineNo),
		  TargetNodeIdx(ToNodeIdx)
	{
	}

	void Reset()
	{
		*this = FSUDSParsedEdge(-1);
	}
	
};

enum class ESUDSParsedNodeType : uint8
{
	/// Text node, displaying a line of dialogue
	Text,
	/// Choice node, displaying a series of user choices which navigate to other nodes
	Choice,
	/// Select node, automatically selecting one which navigates to another node based on state
	Select,
	/// Goto node, redirects execution somewhere else
	/// Gotos are only nodes in the parsing structure, because they need to be discoverable as a fallthrough destination
	/// When converting to runtime use they just become edges
	Goto,
	/// Set variable node
	SetVariable,
};

/// Intermediate parsed node from script text
/// This will be converted into a final asset later
struct SUDSEDITOR_API FSUDSParsedNode
{
public:
	ESUDSParsedNodeType NodeType;
	int OriginalIndent;
	/// Identifier is speaker ID, goto label, variable name etc
	FString Identifier;
	/// Text in native language
	FString Text;
	/// Identifier of the text, for the string table
	FString TextID;
	/// Variable literal value, for set nodes
	FFormatArgumentValue VarLiteral;
	/// Labels which lead to this node
	TArray<FString> Labels;
	/// Edges leading to other nodes
	TArray<FSUDSParsedEdge> Edges;
	/// The line this node was created on
	int SourceLineNo;

	// Path hierarchy of choice/selects leading to this node, of the form "/002/006" etc, not including this node index
	// This helps us identify valid fallthroughs
	FString TreePath;

	FSUDSParsedNode(ESUDSParsedNodeType InNodeType, int Indent, int LineNo) : NodeType(InNodeType), OriginalIndent(Indent), SourceLineNo(LineNo) {}
	FSUDSParsedNode(const FString& InSpeaker, const FString& InText, const FString& InTextID, int Indent, int LineNo)
		:NodeType(ESUDSParsedNodeType::Text), OriginalIndent(Indent), Identifier(InSpeaker), Text(InText), TextID(InTextID), SourceLineNo(LineNo) {}
	FSUDSParsedNode(const FString& GotoLabel, int Indent, int LineNo)
		:NodeType(ESUDSParsedNodeType::Goto), OriginalIndent(Indent), Identifier(GotoLabel), SourceLineNo(LineNo)  {}

	FSUDSParsedNode(const FString& VariableName, const FFormatArgumentValue& LiteralValue, int Indent, int LineNo)
		: NodeType(ESUDSParsedNodeType::SetVariable), OriginalIndent(Indent), Identifier(VariableName), VarLiteral(LiteralValue), SourceLineNo(LineNo) {}
	FSUDSParsedNode(const FString& VariableName, const FFormatArgumentValue& LiteralValue, const FString& InTextID, int Indent, int LineNo)
		: NodeType(ESUDSParsedNodeType::SetVariable), OriginalIndent(Indent), Identifier(VariableName), TextID(InTextID), VarLiteral(LiteralValue), SourceLineNo(LineNo) {}
};
class SUDSEDITOR_API FSUDSScriptImporter
{
public:
	bool ImportFromBuffer(const TCHAR* Buffer, int32 Len, const FString& NameForErrors, bool bSilent);
	void PopulateAsset(USUDSScript* Asset, UStringTable* StringTable);
	static const FString EndGotoLabel;
protected:
	static const FString TreePathSeparator;
	/// Struct for tracking indents
	struct IndentContext
	{
	public:
		// The index of the Node which is the parent of this context
		// This potentially changes every time a sequential text node is encountered in the same context, so it's
		// always pointing to the last node encountered at this level, for connection
		int LastNodeIdx;

		/// The outermost indent level where this context lives
		/// You can indent things that don't create a new context, e.g.
		///   1. Indent a text line under another text line: this is the same as no indent, just a continuation
		///   2. Indent choices or conditions under a text line
		/// This is just good for readability, but does not create a new context, it's just a linear sequence
		/// Therefore the ThresholdIndent tracks the outermost indent relating to the current linear sequence, to know
		/// when you do in fact need to pop the current context off the stack.
		int ThresholdIndent;

		/// The path entry for this indent, to be combined with all previous levels to provide full path context
		FString PathEntry;

		IndentContext(int NodeIdx, int Indent, const FString& Path) : LastNodeIdx(NodeIdx), ThresholdIndent(Indent), PathEntry(Path) {}

	};

	/// A tree of nodes. Contained to separate header nodes from body nodes
	struct ParsedTree
	{
	public:
		/// The indent context stack representing where we are in the indentation tree while parsing
		/// There must always be 1 level (root)
		TArray<IndentContext> IndentLevelStack;
		/// When encountering conditions and choice lines, we are building up details for an edge to another node, but
		/// we currently don't know the target node. We keep these pending details here
		FSUDSParsedEdge* EdgeInProgress = nullptr;
		/// List of all nodes, appended to as parsing progresses
		/// Ordering is important, these nodes must be in the order encountered in the file 
		TArray<FSUDSParsedNode> Nodes;
		/// Record of goto labels to node index, built up during parsing (forward refs are OK so not complete until end of parsing)
		TMap<FString, int> GotoLabelList;
		/// Goto labels which have been encountered but we haven't found a destination yet
		TArray<FString> PendingGotoLabels;
		/// Goto labels that lead directly to another goto and thus are just aliases
		TMap<FString, FString> AliasedGotoLabels;

		void Reset()
		{
			IndentLevelStack.Reset();
			EdgeInProgress = nullptr;
			Nodes.Reset();
			GotoLabelList.Reset();
			PendingGotoLabels.Reset();
			AliasedGotoLabels.Reset();
		}
	};

	ParsedTree HeaderTree;
	ParsedTree BodyTree;

	
	/// List of speakers, detected during parsing of lines of text 
	TArray<FString> ReferencedSpeakers;
	
	const int TabIndentValue = 4;
	bool bHeaderDone = false;
	bool bTooLateForHeader = false;
	bool bHeaderInProgress = false;
	bool bTextInProgress = false;
	int ChoiceUniqueId = 0;
	/// For generating text IDs
	int TextIDHighestNumber = 0;
	/// Parse a single line
	bool ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseHeaderLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseBodyLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseChoiceLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseConditionalLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseGotoLabelLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseGotoLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseSetLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseEventLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseTextLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseLiteral(const FString& ValueStr, FFormatArgumentValue& OutVal);
	bool IsCommentLine(const FStringView& TrimmedLine);
	FStringView TrimLine(const FStringView& Line, int& OutIndentLevel) const;
	void PopIndent(ParsedTree& Tree);
	void PushIndent(ParsedTree& Tree, int NodeIdx, int Indent, const FString& Path);
	FString GetCurrentTreePath(ParsedTree& Tree);
	int AppendNode(ParsedTree& Tree, const FSUDSParsedNode& NewNode);
	void ConnectRemainingNodes(ParsedTree& Tree, const FString& NameForErrors, bool bSilent);
	int FindNextOutdentedNodeIndex(ParsedTree& Tree, int StartNodeIndex, int IndentLessThan, const FString& FromPath);
	void RetrieveAndRemoveOrGenerateTextID(FStringView& InOutLine, FString& OutTextID);
	bool RetrieveAndRemoveTextID(FStringView& InOutLine, FString& OutTextID);
	FString GenerateTextID(const FStringView& Line);
	const FSUDSParsedNode* GetNode(const ParsedTree& Tree, int Index = 0);
	int GetGotoTargetNodeIndex(const ParsedTree& Tree, const FString& InLabel);
	void PopulateAssetFromTree(USUDSScript* Asset,
	                           const ParsedTree& Tree,
	                           TArray<class USUDSScriptNode*>* pOutNodes,
	                           TMap<FName, int>* pOutLabels,
	                           UStringTable* StringTable);

public:
	const FSUDSParsedNode* GetNode(int Index = 0);
	const FSUDSParsedNode* GetHeaderNode(int Index = 0);
	/// Resolve a goto label to a target index (after import), or -1 if not resolvable
	int GetGotoTargetNodeIndex(const FString& Label);
};
