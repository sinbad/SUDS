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
	const TArray<UObject*>& Participants)
{
	if (auto Dlg = CreateDialogue(Owner, Script))
	{
		Dlg->SetParticipants(Participants);
		return Dlg;
	}
	return nullptr;
}

bool USUDSLibrary::GetDialogueValueAsText(const FSUDSValue& Value, FText& TextValue)
{
	if (Value.GetType() == ESUDSValueType::Text)
	{
		TextValue = Value.GetTextValue();
		return true;
	}
	return false;
}

bool USUDSLibrary::GetDialogueValueAsBoolean(const FSUDSValue& Value, bool& BoolValue)
{
	if (Value.GetType() == ESUDSValueType::Boolean)
	{
		BoolValue = Value.GetBooleanValue();
		return true;
	}
	return false;
}

bool USUDSLibrary::GetDialogueValueAsInt(const FSUDSValue& Value, int& IntValue)
{
	if (Value.GetType() == ESUDSValueType::Int)
	{
		IntValue = Value.GetIntValue();
		return true;
	}
	return false;
}

bool USUDSLibrary::GetDialogueValueAsFloat(const FSUDSValue& Value, float& FloatValue)
{
	if (Value.GetType() == ESUDSValueType::Float)
	{
		FloatValue = Value.GetFloatValue();
		return true;
	}
	return false;
}

bool USUDSLibrary::GetDialogueValueAsGender(const FSUDSValue& Value, ETextGender& GenderValue)
{
	if (Value.GetType() == ESUDSValueType::Gender)
	{
		GenderValue = Value.GetGenderValue();
		return true;
	}
	return false;
}
