// Copyright 2020 Old Doorways Ltd


#include "TestParticipant.h"

void UTestParticipant::UpdateDialogueParameters_Implementation(USUDSDialogue* Dialogue, FSUDSTextParameters& Params)
{
	switch(TestNumber)
	{
	default:
	case 0:
		Params.SetParameter("PlayerName", FText::FromString("Protagonist"));
		Params.SetParameter("NPCName", FText::FromString("An NPC"));
		Params.SetParameter("NumCats", 3);
		Params.SetParameter("FriendName", FText::FromString("Susan"));
		Params.SetParameter("Gender", ETextGender::Feminine);
		Params.SetParameter("FloatVal", 12.567);
		Params.SetParameter("BoolVal", true);
		break;
	}
	
	
}
