#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeGosub.generated.h"

/**
 * 
 */
UCLASS()
class SUDS_API USUDSScriptNodeGosub : public USUDSScriptNode
{
	GENERATED_BODY()
protected:
	/// Name of the label which we'll jump to before returning
	UPROPERTY(BlueprintReadOnly)
	FName LabelName;

	/// Generated ID for use when saving state
	UPROPERTY(BlueprintReadOnly)
	FString GosubID;

	/// Convenience flag to let you know whether this node has any choices directly after it
	/// Internally this also lets us know to look for the next choice node after returning
	UPROPERTY(BlueprintReadOnly)
	bool bHasChoices = false;
	
public:

	void Init(const FString& Label, const FString ID, int LineNo)
	{
		NodeType = ESUDSScriptNodeType::Gosub;
		LabelName = FName(Label);
		GosubID = ID;
		SourceLineNo = LineNo;
	}
	FName GetLabelName() const { return LabelName; }
	const FString& GetGosubID() const { return GosubID; }
	bool HasChoices() const { return bHasChoices; }
	
	void NotifyHasChoices() { bHasChoices = true; }

};
