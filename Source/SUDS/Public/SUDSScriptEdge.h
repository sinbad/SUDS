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
public:
	// Text, if a user choice. Always references a string table
	UPROPERTY()
	FText Text;
	
	UPROPERTY()
	TWeakObjectPtr<USUDSScriptNode> TargetNode;

	UPROPERTY()
	ESUDSScriptEdgeNavigation Navigation;
	
	// TODO Add conditions

	FSUDSScriptEdge(): Navigation(ESUDSScriptEdgeNavigation::Explicit) {}

	FSUDSScriptEdge(USUDSScriptNode* ToNode, ESUDSScriptEdgeNavigation Nav) : TargetNode(ToNode), Navigation(Nav) {}

	FSUDSScriptEdge(const FText& InText, USUDSScriptNode* ToNode, ESUDSScriptEdgeNavigation Nav) : Text(InText), TargetNode(ToNode), Navigation(Nav) {}

};