#pragma once

#include "CoreMinimal.h"
#include "SUDSValue.h"
#include "UObject/Object.h"
#include "TestEventSub.generated.h"

class USUDSDialogue;
UCLASS()
class SUDSTEST_API UTestEventSub : public UObject
{
	GENERATED_BODY()

public:
	void Init(USUDSDialogue* Dlg);

	struct FEventRecord
	{
		FName Name;
		TArray<FSUDSValue> Args;
	};

	TArray<FEventRecord> EventRecords;

	UFUNCTION()
	void OnEvent(USUDSDialogue* Dlg, FName EventName, const TArray<FSUDSValue>& Args);
	
	
};
