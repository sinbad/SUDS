#include "SUDSLibrary.h"

#include "SUDSDialogue.h"
#include "SUDSScript.h"
#include "SUDS.h"

USUDSDialogue* USUDSLibrary::CreateDialogue(UObject* Owner, USUDSScript* Script, FName StartAtLabel)
{
	if (IsValid(Script))
	{
		USUDSDialogue* Ret = NewObject<USUDSDialogue>(Owner, Script->GetFName());
		Ret->Initialise(Script, StartAtLabel);
		return Ret;
	}
	UE_LOG(LogSUDS, Error, TEXT("Called CreateDialogue with an invalid script"))
	return nullptr;
}

USUDSDialogue* USUDSLibrary::CreateDialogueWithParticipants(UObject* Owner,
	USUDSScript* Script,
	const TMap<FString, UObject*>& Participants,
	FName StartAtLabel)
{
	if (auto Dlg = CreateDialogue(Owner, Script, StartAtLabel))
	{
		Dlg->SetParticipants(Participants);
		return Dlg;
	}
	return nullptr;
}
