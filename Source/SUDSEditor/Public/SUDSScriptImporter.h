#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"


struct SUDSEDITOR_API FSUDSParsedEdge
{
public:
	/// If an edge is a jump to a label, store the jump target & resolve later
	bool bIsJump = false;
	FString JumpTargetLabel;

	// TODO: Conditions

	/// If not a jump, direct node index link (since should be created immediately afterward)
	int TargetNodeIdx;
	
};

/// Intermediate parsed node from script text
/// This will be converted into a final asset later
struct SUDSEDITOR_API FSUDSParsedNode
{
public:
	ESUDSScriptNodeType NodeType;
	FString Speaker;
	FString Text;
	/// Labels which lead to this node
	TArray<FString> Labels;
	/// Edges leading to other nodes
	TArray<FSUDSParsedEdge> Edges;
	
};
class SUDSEDITOR_API FSUDSScriptImporter
{
public:
	bool ImportFromBuffer(const TCHAR* Buffer, int32 Len, const FString& NameForErrors, bool bSilent);
protected:
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

		IndentContext(int NodeIdx, int Indent) : LastNodeIdx(NodeIdx), ThresholdIndent(Indent) {}

	};
	/// The indent context stack representing where we are in the indentation tree while parsing
	/// There must always be 1 level (root)
	TArray<IndentContext> IndentLevelStack;
	/// List of all nodes, appended to as parsing progresses
	TArray<FSUDSParsedNode> Nodes;
	/// Record of jump labels to node index, built up during parsing (forward refs are OK so not complete until end of parsing)
	TMap<FString, int> JumpList;
	/// List of speakers, declared in header. Used to disambiguate sometimes
	TArray<FString> DeclaredSpeakers;
	/// List of speakers, detected during parsing of lines of text, or events, or get/set variables 
	TArray<FString> ReferencedSpeakers;
	
	const int TabIndentValue = 4;
	bool bHeaderDone = false;
	bool bTooLateForHeader = false;
	bool bHeaderInProgress = false;
	bool bTextInProgress = false;
	/// Parse a single line
	bool ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseHeaderLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseBodyLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseChoiceLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseConditionalLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseGotoLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseSetLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseEventLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseTextLine(const FStringView& Line, int IndentLevel, int LineNo, const FString& String, bool bSilent);
	bool IsCommentLine(const FStringView& TrimmedLine);
	FStringView TrimLine(const FStringView& Line, int& OutIndentLevel) const;
	void PopIndent();
	void PushIndent(int NodeIdx, int Indent);
};
