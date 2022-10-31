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
UCLASS()
class SUDS_API USUDSScriptNode : public UObject
{
	GENERATED_BODY()

protected:

	// Everything UPROPERTY to assist serialization

	/// Type of node
	UPROPERTY()
	ESUDSScriptNodeType NodeType = ESUDSScriptNodeType::Text;
	/// Identifier of the speaker for text nodes
	UPROPERTY(BlueprintReadOnly)
	FString SpeakerID;
protected:
	/// TODO: Replace with TextID when localisation done
	UPROPERTY(BlueprintReadOnly)
	FText TempText;
	/// Identifier of the text string to use (based on the string table used by parent script
	UPROPERTY(BlueprintReadOnly)
	FString TextID;
	/// Links to other nodes
	UPROPERTY(BlueprintReadOnly)
	TArray<FSUDSScriptEdge> Edges;

public:
	USUDSScriptNode();
	
	[[nodiscard]] ESUDSScriptNodeType GetNodeType() const { return NodeType; }
	[[nodiscard]] const FString& GetSpeakerID() const { return SpeakerID; }
	[[nodiscard]] const FString& GetTextID() const { return TextID; }
	[[nodiscard]] const FText& GetTempText() const { return TempText; }
	[[nodiscard]] const TArray<FSUDSScriptEdge>& GetEdges() const { return Edges; }

	void AddEdge(const FSUDSScriptEdge& NewEdge);
	void InitText(const FString& SpeakerID, const FString& TextID);
	void InitChoice();
	void InitSelect();

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
