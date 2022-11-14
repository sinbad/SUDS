#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptEdge.generated.h"

class USUDSScriptNode;

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

	// TODO Add conditions

	mutable bool bFormatExtracted = false; 
	mutable TArray<FName> ParameterNames;
	mutable FTextFormat TextFormat;

	void ExtractFormat() const;
	
public:
	FSUDSScriptEdge() {}

	FSUDSScriptEdge(USUDSScriptNode* ToNode) : TargetNode(ToNode) {}

	FSUDSScriptEdge(const FText& InText, USUDSScriptNode* ToNode) : Text(InText), TargetNode(ToNode) {}

	[[nodiscard]] FText GetText() const { return Text; }
	[[nodiscard]] TWeakObjectPtr<USUDSScriptNode> GetTargetNode() const { return TargetNode; }

	void SetText(const FText& Text);
	void SetTargetNode(const TWeakObjectPtr<USUDSScriptNode>& InTargetNode) { TargetNode = InTargetNode; }

	const FTextFormat& GetTextFormat() const;
	const TArray<FName>& GetParameterNames() const;
	bool HasParameters() const;
};