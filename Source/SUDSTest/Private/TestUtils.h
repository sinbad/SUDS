#pragma once
#include "SUDSDialogue.h"

void TestDialogueText(FAutomationTestBase* T, const FString& NameForTest, USUDSDialogue* D, const FString& SpeakerID, const FString& Text)
{
	T->TestEqual(NameForTest, D->GetSpeakerID(), SpeakerID);
	T->TestEqual(NameForTest, D->GetText().ToString(), Text);
	
}
