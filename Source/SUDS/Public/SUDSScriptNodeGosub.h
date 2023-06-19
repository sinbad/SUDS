// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
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
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	FName LabelName;

	/// Generated ID for use when saving state
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	FString GosubID;

	/// Convenience flag to let you know whether this node MAY HAVE any choices directly after it
	/// Internally this also lets us know to look for the next choice node after returning
	/// It's possible that where there are conditionals ahead, there are only choices on some of the paths.
	/// This flag is to let us know to look for choices, but if conditionals apply we may not find any using actual dialogue state.
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
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
	/// Whether on one select path or another a choice was found
	/// Doesn't help if within a Gosub as call site may be anywhere
	bool MayHaveChoices() const { return bHasChoices; }
	
	void NotifyMayHaveChoices() { bHasChoices = true; }

};
