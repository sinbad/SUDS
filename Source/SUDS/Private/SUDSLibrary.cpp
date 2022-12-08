#include "SUDSLibrary.h"

#include "SUDSDialogue.h"
#include "SUDSScript.h"

USUDSDialogue* USUDSLibrary::CreateDialogue(UObject* Owner, USUDSScript* Script, bool bStartImmediately, FName StartLabel)
{
	if (IsValid(Script))
	{
		if (!IsValid(Owner))
		{
			Owner = GetTransientPackage();
		}
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
	if (IsValid(Script))
	{
		if (!IsValid(Owner))
		{
			Owner = GetTransientPackage();
		}
		// Don't use the base CreateDialogue to start, we want to set participants first before init/start
		USUDSDialogue* Dlg = NewObject<USUDSDialogue>(Owner, Script->GetFName());
		Dlg->SetParticipants(Participants);
		Dlg->Initialise(Script);
		if (bStartImmediately)
		{
			Dlg->Start(StartLabel);
		}
		return Dlg;
	}
	UE_LOG(LogSUDS, Error, TEXT("Called CreateDialogue with an invalid script"))
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

bool USUDSLibrary::GetDialogueValueAsName(const FSUDSValue& Value, FName& NameValue)
{
	if (Value.GetType() == ESUDSValueType::Name)
	{
		NameValue = Value.GetNameValue();
		return true;
	}
	return false;
	
}

ESUDSValueType USUDSLibrary::GetDialogueValueType(const FSUDSValue& Value)
{
	return Value.GetType();
}

bool USUDSLibrary::GetDialogueValueIsEmpty(const FSUDSValue& Value)
{
	return Value.IsEmpty();
}
