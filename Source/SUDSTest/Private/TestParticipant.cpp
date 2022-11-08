// Copyright 2020 Old Doorways Ltd


#include "TestParticipant.h"

#include "SUDSDialogue.h"

void UTestParticipant::OnDialogueStarting_Implementation(USUDSDialogue* Dialogue, FName AtLabel)
{
	switch(TestNumber)
	{
	default:
	case 0:
		Dialogue->SetVariable("PlayerName", FText::FromString("Protagonist"));
		Dialogue->SetVariable("NPCName", FText::FromString("An NPC"));
		Dialogue->SetVariable("NumCats", 3);
		Dialogue->SetVariableText("FriendName", FText::FromString("Susan"));
		Dialogue->SetVariable("Gender", ETextGender::Feminine);
		Dialogue->SetVariableFloat("FloatVal", 12.567);
		Dialogue->SetVariableBoolean("BoolVal", true);
		break;
	}
}

