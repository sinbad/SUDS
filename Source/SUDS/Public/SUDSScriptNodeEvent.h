// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSExpression.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeEvent.generated.h"

/**
 * 
 */
UCLASS()
class SUDS_API USUDSScriptNodeEvent : public USUDSScriptNode
{
	GENERATED_BODY()
protected:
	// Variable identifier
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	FName EventName;
	
	/// Literal arguments
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	TArray<FSUDSExpression> Args;

public:

	void Init(const FString& EvtName, const TArray<FSUDSExpression>& InArgs, int LineNo);
	FName GetEventName() const { return EventName; }
	const TArray<FSUDSExpression>& GetArgs() const { return Args; }
	
	
};
