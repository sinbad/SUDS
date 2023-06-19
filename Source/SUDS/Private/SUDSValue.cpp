// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSValue.h"

FArchive& operator<<(FArchive& Ar, FSUDSValue& Value)
{
	// Custom serialisation since we can't auto-serialise union, TOptional
	uint8 TypeAsInt = (uint8)Value.Type; 
	Ar << TypeAsInt;
	if (Ar.IsLoading())
		Value.Type = static_cast<ESUDSValueType>(TypeAsInt);

	// This gets/sets float value too
	Ar << Value.IntValue;

	if (Value.Type == ESUDSValueType::Text)
	{
		FText Text = Value.TextValue.Get(FText::GetEmpty());
		Ar << Text;
		if (Ar.IsLoading())
			Value.TextValue = Text;
	}
	else if (Value.Type == ESUDSValueType::Variable || Value.Type == ESUDSValueType::Name)
	{
		FString VarNameStr = Value.Name.Get(NAME_None).ToString();
		Ar << VarNameStr;
		if (Ar.IsLoading())
			Value.Name = FName(VarNameStr);
	}
		
	return Ar;
}

void operator<<(FStructuredArchive::FSlot Slot, FSUDSValue& Value)
{
	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	Record
		<< SA_VALUE(TEXT("Type"), Value.Type)
		<< SA_VALUE(TEXT("IntValue"), Value.IntValue); // gets/sets float/boolean/gender too

	if (Value.Type == ESUDSValueType::Text)
	{
		Record << SA_VALUE(TEXT("TextValue"), Value.TextValue);
	}
	else if (Value.Type == ESUDSValueType::Variable || Value.Type == ESUDSValueType::Name)
	{
		Record << SA_VALUE(TEXT("Name"), Value.Name);
	}

}

FString FSUDSValue::ToString() const
{
	switch (Type)
	{
	case ESUDSValueType::Text:
		return GetTextValue().ToString();
	case ESUDSValueType::Int:
		return FString::FromInt(GetIntValue());
	case ESUDSValueType::Float:
		return FString::SanitizeFloat(GetFloatValue());
	case ESUDSValueType::Boolean:
		return GetBooleanValue() ? "True" : "False";
	case ESUDSValueType::Gender:
		return StaticEnum<ETextGender>()->GetNameStringByValue((int64)GetGenderValue());
	case ESUDSValueType::Name:
		return GetNameValue().ToString();
	case ESUDSValueType::Variable:
		return GetVariableNameValue().ToString();
	default:
	case ESUDSValueType::Empty:
		return "Empty";
	}
}

bool FSUDSValue::ExportTextItem(FString& ValueStr,
	FSUDSValue const& DefaultValue,
	UObject* Parent,
	int32 PortFlags,
	UObject* ExportRootScope) const
{
	// This is used to generate the blueprint debugger, but also used in serialisation
	// We need to only implement it for debugging to avoid breaking anything else
	if (0 != (PortFlags & EPropertyPortFlags::PPF_BlueprintDebugView))
	{
		ValueStr.Appendf(TEXT("Type=%s Value=%s"), *StaticEnum<ESUDSValueType>()->GetDisplayValueAsText(Type).ToString(), *ToString());
		return true;
	}

	// Use the default for everything else
	return false;
	
}
