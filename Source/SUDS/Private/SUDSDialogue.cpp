#include "SUDSDialogue.h"

#include "SUDSScript.h"
#include "SUDSScriptNode.h"

USUDSDialogue::USUDSDialogue()
{
}

void USUDSDialogue::Initialise(const USUDSScript* Script, FName StartLabel)
{
	BaseScript = Script;
	if (StartLabel != NAME_None)
	{
		SetCurrentNode(BaseScript->GetNodeByLabel(StartLabel));
	}
	else
	{
		SetCurrentNode(BaseScript->GetFirstNode());
	}
}

void USUDSDialogue::SetCurrentNode(USUDSScriptNode* Node)
{
	CurrentNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	CurrentText = FText::GetEmpty();

}

const FText& USUDSDialogue::GetCurrentText() const
{
	if (CurrentText.IsEmpty())
	{
		// TODO: localisation
	}
	// For now, just use temp text
	return CurrentNode->GetTempText();
}

const FString& USUDSDialogue::GetCurrentSpeakerID() const
{
	return CurrentSpeakerID;
}

const FText& USUDSDialogue::GetCurrentSpeakerDisplayName() const
{
	if (CurrentSpeakerDisplayName.IsEmpty())
	{
		// TODO: derive speaker display name
	}
	return CurrentSpeakerDisplayName;
}

int USUDSDialogue::GetNumberOfChoices() const
{
	// TODO
	return 0;
}

const FText& USUDSDialogue::GetChoiceText(int Index) const
{
	static const FText DummyText = FText::FromString("INVALID");

	// TODO
	return DummyText;
}

bool USUDSDialogue::Continue()
{
	// TODO
	return false;
}

bool USUDSDialogue::Choose(int Index)
{
	// TODO
	return false;
}

bool USUDSDialogue::IsEnded() const
{
	// TODO
	return true;
}
