#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "SUDSValue.h"
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
	FString Identifier;
	
	/// Literal value, for set nodes
	FSUDSValue Literal;

public:

	void Init(const FString& VarName, const FSUDSValue& LiteralValue);
	const FString& GetIdentifier() const { return Identifier; }
	const FSUDSValue& GetLiteral() const { return Literal; }
};
