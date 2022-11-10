#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "Internationalization/Text.h"
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
	FFormatArgumentValue Literal;

public:

	void Init(const FString& VarName, const FFormatArgumentValue& LiteralValue);
	const FString& GetIdentifier() const { return Identifier; }
	const FFormatArgumentValue& GetLiteral() const { return Literal; }
};
