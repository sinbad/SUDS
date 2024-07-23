// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSExpression.h"

struct FSUDSMessageLogger;
class USUDSScript;
DECLARE_LOG_CATEGORY_EXTERN(LogSUDSImporter, Verbose, All);

struct SUDSEDITOR_API FSUDSParsedEdge
{
public:
	/// Text associated with this edge (if a player choice option)
	FString Text;
	/// Identifier of the text, for the string table
	FString TextID;
	/// Metadata associated with text, for translator comments
	TMap<FName, FString> TextMetadata;
	/// The line this edge was created on
	int SourceLineNo;
	/// Condition expression that applies to this edge (for select nodes)
	FSUDSExpression ConditionExpression;

	int SourceNodeIdx = -1;
	int TargetNodeIdx = -1;

	FSUDSParsedEdge(int LineNo) : SourceLineNo(LineNo){}

	FSUDSParsedEdge(int FromNodeIdx, int ToNodeIdx, int LineNo, const FString& InText, const FString& InTextID, const TMap<FName, FString>& Metadata)
		: Text(InText),
		  TextID(InTextID),
		  TextMetadata(Metadata),
		  SourceLineNo(LineNo),
		  SourceNodeIdx(FromNodeIdx),
		  TargetNodeIdx(ToNodeIdx)
	{
	}

	FSUDSParsedEdge(int FromNodeIdx, int ToNodeIdx, int LineNo)
	: SourceLineNo(LineNo),
	  SourceNodeIdx(FromNodeIdx),
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
	/// Select node, automatically selecting one which navigates to another node based on state (also Random)
	Select,
	/// Goto node, redirects execution somewhere else
	/// Gotos are only nodes in the parsing structure, because they need to be discoverable as a fallthrough destination
	/// When converting to runtime use they just become edges
	Goto,
	/// Gosub node, acts like goto except stores return location
	Gosub,
	/// Return node, returns from a gosub
	Return,
	/// Set variable node
	SetVariable,
	/// Event node
	Event
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
	/// Metadata associated with text, for translator comments
	TMap<FName, FString> TextMetadata;
	/// Expression, for nodes that use it (e.g. set)
	FSUDSExpression Expression;
	/// Event arguments, for event nodes
	TArray<FSUDSExpression> EventArgs;
	/// Labels which lead to this node
	TArray<FString> Labels;
	/// Edges leading to other nodes
	TArray<FSUDSParsedEdge> Edges;
	/// The line this node was created on
	int SourceLineNo;
	/// Whether this is a valid fall-through target
	bool AllowFallthrough = true;

	// Path hierarchy of choice nodes leading to this node, of the form "/C002/C006" etc, not including this node index
	// This helps us identify valid fallthroughs
	FString ChoicePath;
	// Path hierarchy of select nodes leading to this node, of the form "/S002/S006" etc, not including this node index
	// This helps us identify valid fallthroughs
	FString ConditionalPath;

	/// Although multiple edges can lead here, this index is for the auto-connected parent (may be nothing)
	int ParentNodeIdx = -1;

	FSUDSParsedNode(ESUDSParsedNodeType InNodeType, int Indent, int LineNo) : NodeType(InNodeType),
	                                                                          OriginalIndent(Indent),
	                                                                          SourceLineNo(LineNo)
	{
	}

	FSUDSParsedNode(const FString& Label,
	                const FString& GosubID,
	                int Indent,
	                int LineNo) : NodeType(ESUDSParsedNodeType::Gosub),
	                              OriginalIndent(Indent),
	                              Identifier(Label),
	                              TextID(GosubID),
	                              SourceLineNo(LineNo)
	{
	}

	FSUDSParsedNode(const FString& InSpeaker, const FString& InText, const FString& InTextID, const TMap<FName, FString>& Metadata, int Indent, int LineNo)
		: NodeType(ESUDSParsedNodeType::Text),
		  OriginalIndent(Indent),
		  Identifier(InSpeaker),
		  Text(InText),
		  TextID(InTextID),
		  TextMetadata(Metadata),
		  SourceLineNo(LineNo)
	{
	}

	FSUDSParsedNode(const FString& GotoLabel, int Indent, int LineNo)
		: NodeType(ESUDSParsedNodeType::Goto), OriginalIndent(Indent), Identifier(GotoLabel), SourceLineNo(LineNo)
	{
	}

	FSUDSParsedNode(const FString& VariableName, const FSUDSExpression& InExpr, int Indent, int LineNo)
		: NodeType(ESUDSParsedNodeType::SetVariable),
		  OriginalIndent(Indent),
		  Identifier(VariableName),
		  Expression(InExpr),
		  SourceLineNo(LineNo)
	{
	}

	FSUDSParsedNode(const FString& VariableName,
	                const FSUDSExpression& InExpr,
	                const FString& InTextID,
	                int Indent,
	                int LineNo)
		: NodeType(ESUDSParsedNodeType::SetVariable),
		  OriginalIndent(Indent),
		  Identifier(VariableName),
		  TextID(InTextID),
		  Expression(InExpr),
		  SourceLineNo(LineNo)
	{
	}
};
class SUDSEDITOR_API FSUDSScriptImporter
{
public:
	bool ImportFromBuffer(const TCHAR* Buffer, int32 Len, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	void PopulateAsset(USUDSScript* Asset, UStringTable* StringTable);
	static FMD5Hash CalculateHash(const TCHAR* Buffer, int32 Len);
	static const FString EndGotoLabel;
protected:
	static const FString TreePathSeparator;

	enum class EConditionalStage : uint8
	{
		IfStage,
		ElseIfStage,
		ElseStage,
		RandomStage,
		RandomOptionStage
	};
	enum class EConditionalParent : uint8
	{
		Select,
		ElseIfStage,
		ElseStage
	};
	
	// Struct for tracking if/elseif blocks
	struct ConditionalContext
	{
		/// Index of parent select node where else will be added
		int SelectNodeIdx = -1;
		/// Previous block index (parent for nesting)
		int PreviousBlockIdx = -1;
		/// Track whether we're in if/elseif/else
		EConditionalStage Stage;
		/// String of current condition 
		FString ConditionStr;

		ConditionalContext(int InSelectNodeIdx, int InPrevBlockIdx, EConditionalStage InStage, const FString& InCondStr) :
			SelectNodeIdx(InSelectNodeIdx),
			PreviousBlockIdx(InPrevBlockIdx),
			Stage(InStage),
			ConditionStr(InCondStr)
		{
		}

	};
	
	/// Struct for tracking indents
	struct IndentContext
	{
	public:
		// The index of the Node which is the parent of this context
		// This potentially changes every time a sequential text node is encountered in the same context, so it's
		// always pointing to the last node encountered at this level, for connection
		int LastNodeIdx = -1;

		/// The outermost indent level where this context lives
		/// You can indent things that don't create a new context, e.g.
		///   1. Indent a text line under another text line: this is the same as no indent, just a continuation
		///   2. Indent choices or conditions under a text line
		/// This is just good for readability, but does not create a new context, it's just a linear sequence
		/// Therefore the ThresholdIndent tracks the outermost indent relating to the current linear sequence, to know
		/// when you do in fact need to pop the current context off the stack.
		int ThresholdIndent = 0;

		int LastTextNodeIdx = -1;

		/// The path entry for this indent, to be combined with all previous levels to provide full path context
		FString PathEntry;

		IndentContext(int NodeIdx, int Indent, const FString& Path) : LastNodeIdx(NodeIdx), ThresholdIndent(Indent), LastTextNodeIdx(-1), PathEntry(Path) {}

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
		int EdgeInProgressNodeIdx = -1;
		int EdgeInProgressEdgeIdx = -1;
		/// List of all nodes, appended to as parsing progresses
		/// Ordering is important, these nodes must be in the order encountered in the file 
		TArray<FSUDSParsedNode> Nodes;
		/// Record of goto labels to node index, built up during parsing (forward refs are OK so not complete until end of parsing)
		TMap<FString, int> GotoLabelList;
		/// Goto labels which have been encountered but we haven't found a destination yet
		TArray<FString> PendingGotoLabels;
		/// Goto labels that lead directly to another goto and thus are just aliases
		TMap<FString, FString> AliasedGotoLabels;

		/// Conditional blocks
		/// "if" creates a new context, uses current as parent
		/// "elseif" and "else" also create new contexts, but copies parent from current (sibling)
		/// "endif" ends the context
		/// You cannot have conditionals that were started in an indent context ending outside it
		TArray<ConditionalContext> ConditionalBlocks;
		/// Index of the current conditional block, if any
		int CurrentConditionalBlockIdx = -1;

		void Reset()
		{
			IndentLevelStack.Reset();
			EdgeInProgressNodeIdx = EdgeInProgressEdgeIdx -1;
			Nodes.Reset();
			GotoLabelList.Reset();
			PendingGotoLabels.Reset();
			AliasedGotoLabels.Reset();
			ConditionalBlocks.Reset();
			CurrentConditionalBlockIdx = -1;
		}
	};

	ParsedTree HeaderTree;
	ParsedTree BodyTree;

	struct ParsedMetadata
	{
	public:
		FName Key;
		FString Value;
		int IndentLevel;

		ParsedMetadata(const FName& InKey, const FString& InValue, int InIndentLevel)
			: Key(InKey),
			  Value(InValue),
			  IndentLevel(InIndentLevel)
		{
		}
	};

	/// Metadata applied to speaker lines / choices until reset
	/// For each key there's a stack of metadata, with most indented at the top
	TMap<FName, TArray<ParsedMetadata>> PersistentMetadata;
	/// Metadata applied just to the next speaker line or choice
	TMap<FName, ParsedMetadata> TransientMetadata;
	
	/// List of speakers, detected during parsing of lines of text 
	TArray<FString> ReferencedSpeakers;
	
	const int TabIndentValue = 4;
	bool bHeaderDone = false;
	bool bTooLateForHeader = false;
	bool bHeaderInProgress = false;
	TOptional<bool> bOverrideGenerateSpeakerLineForChoice;
	TOptional<FString> OverrideChoiceSpeakerID;
	bool bTextInProgress = false;
	int ChoiceUniqueId = 0;
	/// For generating text IDs
	int TextIDHighestNumber = 0;
	/// For generating gosub IDs
	int GosubIDHighestNumber = 0;
	/// Parse a single line
	bool ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool ParseHeaderLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool ParseBodyLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool ParseCommentMetadataLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool IsLastNodeOfType(const ParsedTree& Tree, ESUDSParsedNodeType Type);
	bool ParseChoiceLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger*
	                     Logger,
	                     bool bSilent);
	FSUDSParsedEdge* GetEdgeInProgress(ParsedTree& Tree);
	void EnsureChoiceNodeExistsAboveSelect(ParsedTree& Tree, int IndentLevel, int LineNo);
	static bool IsConditionalLine(const FStringView& Line);
	bool ParseConditionalLine(const FStringView& Line, ParsedTree& Tree, int IndentLevel, int LineNo, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool ParseIfLine(const FStringView& Line,
	                 ParsedTree& Tree,
	                 const FString& ConditionStr,
	                 int IndentLevel,
	                 int LineNo,
	                 const FString& NameForErrors,
	                 FSUDSMessageLogger* Logger,
	                 bool bSilent);
	bool ParseElseIfLine(const FStringView& Line,
	                     ParsedTree& Tree,
	                     const FString& ConditionStr,
	                     int IndentLevel,
	                     int LineNo,
	                     const FString& NameForErrors,
	                     FSUDSMessageLogger* Logger,
	                     bool bSilent);

	bool ParseElseLine(const FStringView& Line,
	                   ParsedTree& Tree,
	                   int IndentLevel,
	                   int LineNo,
	                   const FString& NameForErrors,
	                   FSUDSMessageLogger* Logger,
	                   bool bSilent);
	bool ParseEndIfLine(const FStringView& Line,
	                    ParsedTree& Tree,
	                    int IndentLevel,
	                    int LineNo,
	                    const FString& NameForErrors,
	                    FSUDSMessageLogger*
	                    Logger,
	                    bool bSilent);
	static bool IsRandomLine(const FStringView& Line);
	bool ParseRandomLine(const FStringView& Line,
	                     ParsedTree& Tree,
	                     int IndentLevel,
	                     int LineNo,
	                     const FString& NameForErrors,
	                     FSUDSMessageLogger* Logger,
	                     bool bSilent);
	bool ParseBeginRandomLine(const FStringView& Line,
						 ParsedTree& Tree,
						 int IndentLevel,
						 int LineNo,
						 const FString& NameForErrors,
						 FSUDSMessageLogger* Logger,
						 bool bSilent);
	bool ParseRandomOptionLine(const FStringView& Line,
						 ParsedTree& Tree,
						 int IndentLevel,
						 int LineNo,
						 const FString& NameForErrors,
						 FSUDSMessageLogger* Logger,
						 bool bSilent);
	bool ParseEndRandomLine(const FStringView& Line,
						 ParsedTree& Tree,
						 int IndentLevel,
						 int LineNo,
						 const FString& NameForErrors,
						 FSUDSMessageLogger* Logger,
						 bool bSilent);
	bool ParseGotoLabelLine(const FStringView& Line,
	                        ParsedTree& Tree,
	                        int IndentLevel,
	                        int LineNo,
	                        const FString& NameForErrors,
	                        FSUDSMessageLogger* Logger,
	                        bool bSilent);
	bool ParseGotoLine(const FStringView& Line,
	                   ParsedTree& Tree,
	                   int IndentLevel,
	                   int LineNo,
	                   const FString& NameForErrors,
	                   FSUDSMessageLogger* Logger,
	                   bool bSilent);
	bool ParseGosubLine(const FStringView& Line,
	                    ParsedTree& Tree,
	                    int IndentLevel,
	                    int LineNo,
	                    const FString& NameForErrors,
	                    FSUDSMessageLogger* Logger,
	                    bool bSilent);
	bool ParseReturnLine(const FStringView& Line,
	                     ParsedTree& Tree,
	                     int IndentLevel,
	                     int LineNo,
	                     const FString& NameForErrors,
	                     FSUDSMessageLogger* Logger,
	                     bool bSilent);
	bool ParseSetLine(const FStringView& Line,
	                  ParsedTree& Tree,
	                  int IndentLevel,
	                  int LineNo,
	                  const FString& NameForErrors,
	                  FSUDSMessageLogger* Logger,
	                  bool bSilent);
	bool ParseEventLine(const FStringView& Line,
	                    ParsedTree& Tree,
	                    int IndentLevel,
	                    int LineNo,
	                    const FString& NameForErrors,
	                    FSUDSMessageLogger* Logger,
	                    bool bSilent);
	bool ParseTextLine(const FStringView& Line,
	                   ParsedTree& Tree,
	                   int IndentLevel,
	                   int LineNo,
	                   const FString& NameForErrors,
	                   FSUDSMessageLogger* Logger,
	                   bool bSilent);
	bool ParseImportSettingLine(const FStringView& Line,
	                            ParsedTree& Tree,
	                            int IndentLevel,
	                            int LineNo,
	                            const FString& NameForErrors,
	                            FSUDSMessageLogger* Logger,
	                            bool bSilent);
	TMap<FName, FString> GetTextMetadataForNextEntry(int CurrentLineIndent);
	bool IsCommentLine(const FStringView& TrimmedLine);
	FStringView TrimLine(const FStringView& Line, int& OutIndentLevel) const;
	int FindChoiceAfterTextNode(const FSUDSScriptImporter::ParsedTree& Tree, int TextNodeIdx);
	int FindLastChoiceNode(const ParsedTree& Tree, int IndentLevel);
	int FindLastChoiceNode(const ParsedTree& Tree, int IndentLevel, int FromIndex, const FString& ConditionPath);
	void PopIndent(ParsedTree& Tree);
	void PushIndent(ParsedTree& Tree, int NodeIdx, int Indent, const FString& Path);
	FString GetCurrentTreePath(const FSUDSScriptImporter::ParsedTree& Tree);
	FString GetCurrentTreeConditionalPath(const FSUDSScriptImporter::ParsedTree& Tree);
	void SetFallthroughForNewNode(FSUDSScriptImporter::ParsedTree& Tree, FSUDSParsedNode& NewNode);
	int AppendNode(ParsedTree& Tree, const FSUDSParsedNode& InNode);
	bool SelectNodeIsMissingElsePath(const FSUDSScriptImporter::ParsedTree& Tree, const FSUDSParsedNode& Node);
	bool PostImportSanityCheck(const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool ChoiceNodeCheckPaths(const FSUDSParsedNode& ChoiceNode,
	                          const FString& NameForErrors,
	                          FSUDSMessageLogger* Logger,
	                          bool bSilent);
	bool RecurseChoiceNodeCheckPaths(const FSUDSParsedNode& OrigChoiceNode, const FSUDSParsedNode& CurrChoiceNode, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	bool RecurseChoiceNodeCheckPaths(const FSUDSParsedNode& ChoiceNode, const FSUDSParsedEdge& Edge, const FString& NameForErrors, FSUDSMessageLogger*
	                                 Logger,
	                                 bool bSilent);
	void ConnectRemainingNodes(ParsedTree& Tree, const FString& NameForErrors, FSUDSMessageLogger* Logger, bool bSilent);
	void GenerateTextIDs(ParsedTree& BodyTree);
	int FindFallthroughNodeIndex(ParsedTree& Tree, int StartNodeIndex, const FString& FromChoicePath, const FString& FromConditionalPath);
	bool RetrieveAndRemoveTextID(FStringView& InOutLine, FString& OutTextID);
	bool RetrieveAndRemoveGosubID(FStringView& InOutLine, FString& OutTextID);
	FString GenerateTextID();
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
	static bool RetrieveTextIDFromLine(FStringView& InOutLine, FString& OutTextID, int& OutNumber);
	static bool RetrieveGosubIDFromLine(FStringView& InOutLine, FString& OutID, int& OutNumber);
};
