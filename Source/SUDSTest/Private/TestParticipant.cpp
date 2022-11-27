
#include "TestParticipant.h"

#include "SUDSDialogue.h"

void UTestParticipant::OnDialogueStarting_Implementation(USUDSDialogue* Dialogue, FName AtLabel)
{
	switch(TestNumber)
	{
	default:
	case 0:
		Dialogue->SetVariable("SpeakerName.Player", FText::FromString("Protagonist"));
		Dialogue->SetVariable("SpeakerName.NPC", FText::FromString("An NPC"));
		Dialogue->SetVariable("NumCats", 3);
		Dialogue->SetVariableText("FriendName", FText::FromString("Susan"));
		Dialogue->SetVariable("Gender", ETextGender::Feminine);
		Dialogue->SetVariableFloat("FloatVal", 12.567);
		Dialogue->SetVariableBoolean("BoolVal", true);
		break;
	case 1:
		Dialogue->SetVariable("SpeakerName.Player", FText::FromString("Hero"));
		Dialogue->SetVariable("SpeakerName.NPC", FText::FromString("Bob The NPC"));
		Dialogue->SetVariableText("FriendName", FText::FromString("Derek"));
		Dialogue->SetVariable("Gender", ETextGender::Masculine);
		Dialogue->SetVariable("NumCats", 5);
		break;
	case 2:
		// these will all be overridden by higher priorities
		Dialogue->SetVariable("SpeakerName.Player", FText::FromString("Dweeb"));
		Dialogue->SetVariable("SpeakerName.NPC", FText::FromString("Imaginary Friend"));
		Dialogue->SetVariable("NumCats", 10);
		Dialogue->SetVariableFloat("FloatVal", 0.002);
		// This one will be unique and so will still get through
		Dialogue->SetVariableInt("SomethingUniqueTo3", 120);
		break;
	}
}

int UTestParticipant::GetDialogueParticipantPriority_Implementation() const
{
	switch(TestNumber)
	{
	default:
	case 0:
		return 0;
	case 1:
		return 100;
	case 2:
		return -200;
			
	}
}

void UTestParticipant::OnDialogueEvent_Implementation(USUDSDialogue* Dialogue,
	FName EventName,
	const TArray<FSUDSValue>& Arguments)
{
	EventRecords.Add(FEventRecord { EventName, Arguments });
}

void UTestParticipant::OnDialogueVariableChanged_Implementation(USUDSDialogue* Dialogue,
	FName VariableName,
	const FSUDSValue& Value,
	bool bFromScript)
{
	SetVarRecords.Add(FSetVarRecord { VariableName, Value, bFromScript });
}

