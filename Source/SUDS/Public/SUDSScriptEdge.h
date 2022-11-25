#pragma once

#include "CoreMinimal.h"
#include "SUDSExpression.h"
#include "SUDSScriptEdge.generated.h"

class USUDSScriptNode;

UENUM(BlueprintType)
enum class ESUDSEdgeType : uint8
{
	/// A simple continuation; usually for sequences with no choices
	Continue,
	/// A decision made by the player from a list of choices
	Decision,
	/// A conditional path, taken automatically based on the situation
	Condition,
	/// An edge which forms a chain of nodes which are supposed to be considered together
	/// This links a text node to its choices, and also potentially compound choices underneath if there are selects 
	Chained
	
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
	ESUDSEdgeType Type;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<USUDSScriptNode> TargetNode;

	UPROPERTY(BlueprintReadOnly)
	FSUDSExpression Condition;

	UPROPERTY(BlueprintReadOnly)
	int SourceLineNo;

	mutable bool bFormatExtracted = false; 
	mutable TArray<FName> ParameterNames;
	mutable FTextFormat TextFormat;

	void ExtractFormat() const;
	
public:
	FSUDSScriptEdge(): Type(ESUDSEdgeType::Continue)
	{
	}

	FSUDSScriptEdge(USUDSScriptNode* ToNode, ESUDSEdgeType InType, int LineNo) : Type(InType),
		TargetNode(ToNode),
		SourceLineNo(LineNo)
	{
	}

	FSUDSScriptEdge(const FText& InText, USUDSScriptNode* ToNode, int LineNo) : Text(InText),
		Type(ESUDSEdgeType::Decision),
		TargetNode(ToNode),
		SourceLineNo(LineNo)
	{
	}

	FText GetText() const { return Text; }
	ESUDSEdgeType GetType() const { return Type; }
	TWeakObjectPtr<USUDSScriptNode> GetTargetNode() const { return TargetNode; }
	const FSUDSExpression& GetCondition() const { return Condition; }
	int GetSourceLineNo() const { return SourceLineNo; }

	void SetText(const FText& Text);
	void SetType(ESUDSEdgeType InType) { Type = InType; } 
	void SetTargetNode(const TWeakObjectPtr<USUDSScriptNode>& InTargetNode) { TargetNode = InTargetNode; }
	void SetCondition(const FSUDSExpression& InCondition) { Condition = InCondition; }

	const FTextFormat& GetTextFormat() const;
	const TArray<FName>& GetParameterNames() const;
	bool HasParameters() const;
};