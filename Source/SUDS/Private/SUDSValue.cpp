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
	else if (Value.Type == ESUDSValueType::Variable)
	{
		FString VarNameStr = Value.VariableName.Get(NAME_None).ToString();
		Ar << VarNameStr;
		if (Ar.IsLoading())
			Value.VariableName = FName(VarNameStr);
	}
		
	return Ar;
}
