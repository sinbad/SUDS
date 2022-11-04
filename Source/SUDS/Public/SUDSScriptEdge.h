#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptEdge.generated.h"

class USUDSScriptNode;

/// How / when an edge is navigated
UENUM(BlueprintType)
enum class ESUDSScriptEdgeNavigation : uint8
{
	/// Combine with previous, e.g. a choice node hanging off a text node
	Combine = 0,
	/// Navigate when this option is explicitly chosen (even if it's the only edge, requires explicit action to proceed])
	Explicit = 1,
	/// Navigate after a timeout
	Timeout = 2,
	/// Poll regularly and navigate when conditions become true
	Conditional = 3
	
};
/**
* Edge in the script graph. An edge leads to another node (unidirectional)
* Edges can have conditions which mean whether they're valid or not, either as automatic
* choices or player choices.
*/
USTRUCT(BlueprintType)
struct SUDS_API FSUDSScriptEdge
{
	GENERATED_BODY()
protected:
	// Text, if a user choice. Always references a string table
	UPROPERTY(BlueprintReadOnly)
	FText Text;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<USUDSScriptNode> TargetNode;

	UPROPERTY(BlueprintReadOnly)
	ESUDSScriptEdgeNavigation Navigation;
	
	// TODO Add conditions

	mutable bool bFormatExtracted = false; 
	mutable TArray<FString> ParameterNames;
	mutable FTextFormat TextFormat;

	void ExtractFormat() const;
	
public:
	FSUDSScriptEdge(): Navigation(ESUDSScriptEdgeNavigation::Explicit) {}

	FSUDSScriptEdge(USUDSScriptNode* ToNode, ESUDSScriptEdgeNavigation Nav) : TargetNode(ToNode), Navigation(Nav) {}

	FSUDSScriptEdge(const FText& InText, USUDSScriptNode* ToNode, ESUDSScriptEdgeNavigation Nav) : Text(InText), TargetNode(ToNode), Navigation(Nav) {}

	[[nodiscard]] FText GetText() const { return Text; }
	[[nodiscard]] TWeakObjectPtr<USUDSScriptNode> GetTargetNode() const { return TargetNode; }
	[[nodiscard]] ESUDSScriptEdgeNavigation GetNavigation() const { return Navigation; }

	void SetText(const FText& Text);
	void SetTargetNode(const TWeakObjectPtr<USUDSScriptNode>& InTargetNode) { TargetNode = InTargetNode; }
	void SetNavigation(ESUDSScriptEdgeNavigation InNavigation) { Navigation = InNavigation; }

	const FTextFormat& GetTextFormat() const;
	const TArray<FString>& GetParameterNames() const;	
	
};