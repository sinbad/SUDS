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
	
	
};
