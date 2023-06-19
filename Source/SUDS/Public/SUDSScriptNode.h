// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptEdge.h"
#include "UObject/Object.h"
#include "SUDSScriptNode.generated.h"

UENUM(BlueprintType)
enum class ESUDSScriptNodeType : uint8
{
	/// Text node, displaying a line of dialogue
	Text,
	/// Choice node, displaying a series of user choices which navigate to other nodes
	Choice,
	/// Select node, automatically selecting one which navigates to another node based on state
	Select,
	/// Set variable node
	SetVariable,
	/// Event node
	Event,
	/// Gosub node
	Gosub,
	/// Return node
	Return,
};
/**
 * A node in the script graph.
 * Nodes are either text, or branch points (user choice or automatic branching logic)
 * Text nodes always lead to a single next step, be that another text node or a branch.
 * Branch nodes are separate from the text so that jumping can return to a choice point without emitting more text.
 * At runtime if a text node's next step is a user choice, it will be available immediately. Otherwise it's not
 * evaluated until the dialogue progresses.
 * Edges connect everything. Edges may have text if they're user choices (even if that's a single choice), and
 * may have conditions.
 */
UCLASS(BlueprintType)
class SUDS_API USUDSScriptNode : public UObject
{
	GENERATED_BODY()

protected:

	/// Type of node
	/// To make it easier to check rather than having to cast to subtypes blindly. And also not all types need a subtype
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	ESUDSScriptNodeType NodeType = ESUDSScriptNodeType::Text;
	/// Links to other nodes
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	TArray<FSUDSScriptEdge> Edges;
	/// The line number in the script that this node came from
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	int SourceLineNo;


public:
	USUDSScriptNode();

	ESUDSScriptNodeType GetNodeType() const { return NodeType; }
	const TArray<FSUDSScriptEdge>& GetEdges() const { return Edges; }
	int GetSourceLineNo() const { return SourceLineNo; }

	void AddEdge(const FSUDSScriptEdge& NewEdge);
	void InitChoice(int LineNo);
	void InitSelect(int LineNo);
	void InitReturn(int LineNo);

	int GetEdgeCount() const { return Edges.Num(); }
	const FSUDSScriptEdge* GetEdge(int Index) const
	{
		if (Edges.IsValidIndex(Index))
		{
			return &Edges[Index];			
		}
		return nullptr;
	}

};
