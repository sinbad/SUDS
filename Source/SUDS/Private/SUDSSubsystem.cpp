// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSSubsystem.h"
#include "Sound/SoundConcurrency.h"

DEFINE_LOG_CATEGORY(LogSUDSSubsystem)

#if WITH_EDITORONLY_DATA
	TMap<FName, FSUDSValue> USUDSSubsystem::Test_DummyGlobalVariables;
#endif

void USUDSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default to a single voice line being played at once
	VoiceConcurrency = NewObject<USoundConcurrency>(this);
	VoiceConcurrency->Concurrency.MaxCount = 1;
}

void USUDSSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USUDSSubsystem::SetMaxConcurrentVoicedLines(int ConcurrentLines)
{
	if (IsValid(VoiceConcurrency))
	{
		VoiceConcurrency->Concurrency.MaxCount = ConcurrentLines;
	}
}

int USUDSSubsystem::GetMaxConcurrentVoicedLines() const
{
	if (IsValid(VoiceConcurrency))
	{
		return VoiceConcurrency->Concurrency.MaxCount;
	}
	return 1;
}

void USUDSSubsystem::SetVoicedLineConcurrencySettings(const FSoundConcurrencySettings& NewSettings)
{
	if (IsValid(VoiceConcurrency))
	{
		VoiceConcurrency->Concurrency = NewSettings;
	}
}

const FSoundConcurrencySettings& USUDSSubsystem::GetVoicedLineConcurrencySettings() const
{
	if (IsValid(VoiceConcurrency))
	{
		return VoiceConcurrency->Concurrency;
	}
	
	static const FSoundConcurrencySettings Dummy;
	return Dummy;
	
}

void USUDSSubsystem::ResetGlobalState(bool bResetVariables)
{
	if (bResetVariables)
		GlobalVariableState.Empty();
}

FSUDSGlobalState USUDSSubsystem::GetSavedGlobalState() const
{
	return FSUDSGlobalState(GlobalVariableState);
}

void USUDSSubsystem::RestoreSavedGlobalState(const FSUDSGlobalState& State)
{
	ResetGlobalState();
	GlobalVariableState.Append(State.GetGlobalVariables());
}


FText USUDSSubsystem::GetGlobalVariableText(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		if (Arg->GetType() == ESUDSValueType::Text)
		{
			return Arg->GetTextValue();
		}
		else
		{
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Requested variable %s of type text but was not a compatible type"), *Name.ToString());
		}
	}
	return FText();
}

void USUDSSubsystem::SetGlobalVariableInt(FName Name, int32 Value)
{
	SetGlobalVariable(Name, Value);
}

int USUDSSubsystem::GetGlobalVariableInt(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Int:
			return Arg->GetIntValue();
		case ESUDSValueType::Float:
			UE_LOG(LogSUDSSubsystem, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name.ToString());
			return Arg->GetFloatValue();
		default: 
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Variable %s is not a compatible integer type"), *Name.ToString());
		}
	}
	return 0;
}

void USUDSSubsystem::SetGlobalVariableFloat(FName Name, float Value)
{
	SetGlobalVariable(Name, Value);
}

float USUDSSubsystem::GetGlobalVariableFloat(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Int:
			return Arg->GetIntValue();
		case ESUDSValueType::Float:
			return Arg->GetFloatValue();
		default: 
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Variable %s is not a compatible float type"), *Name.ToString());
		}
	}
	return 0;
}

void USUDSSubsystem::SetGlobalVariableGender(FName Name, ETextGender Value)
{
	SetGlobalVariable(Name, Value);
}

ETextGender USUDSSubsystem::GetGlobalVariableGender(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Gender:
			return Arg->GetGenderValue();
		default: 
		case ESUDSValueType::Int:
		case ESUDSValueType::Float:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Variable %s is not a compatible gender type"), *Name.ToString());
		}
	}
	return ETextGender::Neuter;
}

void USUDSSubsystem::SetGlobalVariableBoolean(FName Name, bool Value)
{
	// Use explicit FSUDSValue constructor to avoid default int conversion
	SetGlobalVariable(Name, FSUDSValue(Value));
}

bool USUDSSubsystem::GetGlobalVariableBoolean(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Boolean:
			return Arg->GetBooleanValue();
		case ESUDSValueType::Int:
			return Arg->GetIntValue() != 0;
		default: 
		case ESUDSValueType::Float:
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Variable %s is not a compatible boolean type"), *Name.ToString());
		}
	}
	return false;
}

void USUDSSubsystem::SetGlobalVariableName(FName Name, FName Value)
{
	SetGlobalVariable(Name, FSUDSValue(Value, false));
}

FName USUDSSubsystem::GetGlobalVariableName(FName Name) const
{
	if (const auto Arg = GlobalVariableState.Find(Name))
	{
		if (Arg->GetType() == ESUDSValueType::Name)
		{
			return Arg->GetNameValue();
		}
		else
		{
			UE_LOG(LogSUDSSubsystem, Error, TEXT("Requested variable %s of type text but was not a compatible type"), *Name.ToString());
		}
	}
	return NAME_None;
}

void USUDSSubsystem::UnSetGlobalVariable(FName Name)
{
	GlobalVariableState.Remove(Name);
}
