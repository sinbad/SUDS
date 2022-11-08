#include "SUDSLibrary.h"

#include "SUDSDialogue.h"
#include "SUDSScript.h"
#include "SUDS.h"

USUDSDialogue* USUDSLibrary::CreateDialogue(UObject* Owner, USUDSScript* Script)
{
	if (IsValid(Script))
	{
		USUDSDialogue* Ret = NewObject<USUDSDialogue>(Owner, Script->GetFName());
		Ret->Initialise(Script);
		return Ret;
	}
	UE_LOG(LogSUDS, Error, TEXT("Called CreateDialogue with an invalid script"))
	return nullptr;
}

USUDSDialogue* USUDSLibrary::CreateDialogueWithParticipants(UObject* Owner,
	USUDSScript* Script,
	const TMap<FString, UObject*>& Participants)
{
	if (auto Dlg = CreateDialogue(Owner, Script))
	{
		Dlg->SetParticipants(Participants);
		return Dlg;
	}
	return nullptr;
}
