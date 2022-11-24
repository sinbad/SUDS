#include "SUDSLibrary.h"

#include "SUDSDialogue.h"
#include "SUDSScript.h"

USUDSDialogue* USUDSLibrary::CreateDialogue(UObject* Owner, USUDSScript* Script, bool bStartImmediately, FName StartLabel)
{
	if (IsValid(Script))
	{
		USUDSDialogue* Ret = NewObject<USUDSDialogue>(Owner, Script->GetFName());
		Ret->Initialise(Script);
		if (bStartImmediately)
		{
			Ret->Start(StartLabel);
		}
		return Ret;
	}
	UE_LOG(LogSUDS, Error, TEXT("Called CreateDialogue with an invalid script"))
	return nullptr;
}

USUDSDialogue* USUDSLibrary::CreateDialogueWithParticipants(UObject* Owner,
	USUDSScript* Script,
	const TArray<UObject*>& Participants, bool bStartImmediately, FName StartLabel)
{
	// Don't use the base CreateDialogue to start, we want to set participants first
	if (auto Dlg = CreateDialogue(Owner, Script, false))
	{
		Dlg->SetParticipants(Participants);
		if (bStartImmediately)
		{
			Dlg->Start(StartLabel);
		}
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
