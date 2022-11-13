// Copyright 2020 Old Doorways Ltd

#pragma once

#include "CoreMinimal.h"
#include "SUDSParticipant.h"
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

	
	virtual void OnDialogueStarting_Implementation(USUDSDialogue* Dialogue, FName AtLabel) override;
	virtual int GetDialogueParticipantPriority_Implementation() const override;
};
