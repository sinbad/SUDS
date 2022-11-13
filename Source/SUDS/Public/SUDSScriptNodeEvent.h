#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "SUDSValue.h"
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
	UPROPERTY(BlueprintReadOnly)
	FString EventName;
	
	/// Literal arguments
	UPROPERTY(BlueprintReadOnly)
	TArray<FSUDSValue> LiteralArgs;

public:

	void Init(const FString& EvtName, const TArray<FSUDSValue>& Args);
	const FString& GetEventName() const { return EventName; }
	const TArray<FSUDSValue>& GetLiteralArgs() const { return LiteralArgs; }
	
	
};
