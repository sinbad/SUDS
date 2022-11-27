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
	struct FSetVarRecord
	{
		FName Name;
		FSUDSValue Value;
		bool bFromScript;
	};

	TArray<FEventRecord> EventRecords;
	TArray<FSetVarRecord> SetVarRecords;

	UFUNCTION()
	void OnEvent(USUDSDialogue* Dlg, FName EventName, const TArray<FSUDSValue>& Args);

	UFUNCTION()
	void OnVariableChanged(USUDSDialogue* Dlg, FName VarName, const FSUDSValue& Value, bool bFromScript);

	
};
