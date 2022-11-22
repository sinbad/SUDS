#pragma once

#include "CoreMinimal.h"
#include "SUDSExpression.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeSet.generated.h"

/**
* Set variable node 
*/
UCLASS()
class SUDS_API USUDSScriptNodeSet : public USUDSScriptNode
{
	GENERATED_BODY()

protected:
	// Variable identifier
	UPROPERTY(BlueprintReadOnly)
	FName Identifier;
	
	/// Expression to provide value to set
	UPROPERTY(BlueprintReadOnly)
	FSUDSExpression Expression;

public:

	void Init(const FString& VarName, const FSUDSExpression& InExpression);
	const FName& GetIdentifier() const { return Identifier; }
	const FSUDSExpression& GetExpression() const { return Expression; }
	
};
