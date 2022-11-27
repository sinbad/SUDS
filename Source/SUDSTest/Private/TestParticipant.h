#pragma once

#include "CoreMinimal.h"
#include "SUDSParticipant.h"
#include "SUDSValue.h"
#include "UObject/Object.h"
#include "TestParticipant.generated.h"

/**
 * 
 */
UCLASS()
class SUDSTEST_API UTestParticipant : public UObject, public ISUDSParticipant
{
	GENERATED_BODY()

public:
	int TestNumber = 0;

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

	
	virtual void OnDialogueStarting_Implementation(USUDSDialogue* Dialogue, FName AtLabel) override;
	virtual int GetDialogueParticipantPriority_Implementation() const override;
	virtual void OnDialogueEvent_Implementation(USUDSDialogue* Dialogue,
		FName EventName,
		const TArray<FSUDSValue>& Arguments) override;
	virtual void OnDialogueVariableChanged_Implementation(USUDSDialogue* Dialogue,
		FName VariableName,
		const FSUDSValue& Value,
		bool bFromScript) override;
};
