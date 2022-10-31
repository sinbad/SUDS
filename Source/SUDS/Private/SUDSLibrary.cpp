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
