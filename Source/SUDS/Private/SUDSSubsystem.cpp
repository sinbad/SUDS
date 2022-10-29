#include "SUDSSubsystem.h"

#include "SUDSDialogue.h"
#include "SUDSScript.h"


DEFINE_LOG_CATEGORY(LogSUDSSubsystem)

void USUDSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USUDSSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

USUDSDialogue* USUDSSubsystem::CreateDialogue(UObject* Owner, USUDSScript* Script, FName StartAtLabel)
{
	if (IsValid(Script))
	{
		USUDSDialogue* Ret = NewObject<USUDSDialogue>(Owner, Script->GetFName());
		Ret->Initialise(Script, StartAtLabel);
		return Ret;
	}
	UE_LOG(LogSUDSSubsystem, Error, TEXT("Called CreateDialogue with an invalid script"))
	return nullptr;
}
