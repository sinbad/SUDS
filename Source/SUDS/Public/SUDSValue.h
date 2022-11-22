#pragma once

#include "SUDSCommon.h"
#include "SUDSValue.generated.h"


UENUM(BlueprintType)
enum class ESUDSValueType : uint8
{
	Text,
	Int,
	Float,
	Boolean,
	Gender,
	/// Access the value of another variable
	Variable,
};
/// Struct which can hold any of the value types that SUDS needs to use, in a Blueprint friendly manner
/// For getting / setting these values from blueprints, see blueprint library functions SetSUDSValue<Type>() / GetSUDSValue<Type>()
/// For convenience these are wrapped in USUDSDialogue but in e.g. event callbacks they're not
USTRUCT(BlueprintType)
struct FSUDSValue
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly)
	ESUDSValueType Type;
	union
	{
		int32 IntValue;
		float FloatValue;
	};
	TOptional<FText> TextValue;
	TOptional<FName> VariableName;
public:

	FSUDSValue() : Type(ESUDSValueType::Text), IntValue(0), TextValue(FText::GetEmpty()) {}

	FSUDSValue(const int32 Value)
		: Type(ESUDSValueType::Int) { IntValue = Value; }

	FSUDSValue(const float Value)
		: Type(ESUDSValueType::Float) { FloatValue = Value; }

	FSUDSValue(const FText& Value)
		: Type(ESUDSValueType::Text),
		  IntValue(0),
		  TextValue(Value)
	{
	}

	FSUDSValue(FText&& Value)
		: Type(ESUDSValueType::Text), IntValue(0), TextValue(MoveTemp(Value))
	{
	}

	FSUDSValue(ETextGender Value)
		: Type(ESUDSValueType::Gender), IntValue(0)
	{
		IntValue = static_cast<int32>(Value);
	}

	explicit FSUDSValue(bool Value)
		: Type(ESUDSValueType::Boolean), IntValue(0)
	{
		IntValue = Value ? 1 : 0;
	}

	FSUDSValue(const FName& ReferencedVariableName)
	: Type(ESUDSValueType::Variable),
	  IntValue(0),
	  VariableName(ReferencedVariableName)
	{
	}

	FORCEINLINE ESUDSValueType GetType() const
	{
		return Type;
	}

	FORCEINLINE int32 GetIntValue() const
	{
		if (Type != ESUDSValueType::Int)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as int but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return IntValue;
	}

	FORCEINLINE float GetFloatValue() const
	{
		if (Type != ESUDSValueType::Float)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as float but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return FloatValue;
	}

	FORCEINLINE const FText& GetTextValue() const
	{
		if (Type != ESUDSValueType::Text)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as text but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return TextValue.GetValue();
	}

	FORCEINLINE ETextGender GetGenderValue() const
	{
		if (Type != ESUDSValueType::Gender)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as float but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return static_cast<ETextGender>(IntValue);
	}

	FORCEINLINE bool GetBooleanValue() const
	{
		if (Type != ESUDSValueType::Boolean)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as boolean but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		return IntValue != 0;
	}

	FORCEINLINE FName GetVariableNameValue() const
	{
		if (Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as variable name but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		return VariableName.GetValue();
	}

	FORCEINLINE bool IsVariable() const
	{
		return Type == ESUDSValueType::Variable;
	}

	bool IsNumeric() const
	{
		return Type == ESUDSValueType::Float || Type == ESUDSValueType::Int;
	}

	FFormatArgumentValue ToFormatArg() const
	{
		switch (Type)
		{
		default:
		case ESUDSValueType::Text:
			return FFormatArgumentValue(TextValue.GetValue());
		case ESUDSValueType::Int:
			return FFormatArgumentValue(GetIntValue());
		case ESUDSValueType::Boolean:
			return FFormatArgumentValue(GetBooleanValue());
		case ESUDSValueType::Gender:
			return FFormatArgumentValue(GetGenderValue());
		case ESUDSValueType::Float:
			return FFormatArgumentValue(GetFloatValue());
		}
	}

	void SetValue(const FSUDSValue& RValue)
	{
		*this = RValue;
	}

	/// Not operation, only valid on booleans
	FSUDSValue operator!() const
	{
		check(Type == ESUDSValueType::Boolean);
		return FSUDSValue(!GetBooleanValue());
	}

	FSUDSValue operator*(const FSUDSValue& Rhs) const
	{
		check(IsNumeric() && Rhs.IsNumeric());
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() * Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Int ? GetIntValue() : GetFloatValue())
			*
			(Rhs.GetType() == ESUDSValueType::Int ? Rhs.GetIntValue() : Rhs.GetFloatValue()));
	}
	FSUDSValue operator/(const FSUDSValue& Rhs) const
	{
		check(IsNumeric() && Rhs.IsNumeric());
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() / Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Int ? GetIntValue() : GetFloatValue())
			/
			(Rhs.GetType() == ESUDSValueType::Int ? Rhs.GetIntValue() : Rhs.GetFloatValue()));
	}
	FSUDSValue operator+(const FSUDSValue& Rhs) const
	{
		check(IsNumeric() && Rhs.IsNumeric());
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() + Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Int ? GetIntValue() : GetFloatValue())
			+
			(Rhs.GetType() == ESUDSValueType::Int ? Rhs.GetIntValue() : Rhs.GetFloatValue()));
	}
	FSUDSValue operator-(const FSUDSValue& Rhs) const
	{
		check(IsNumeric() && Rhs.IsNumeric());
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() - Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Int ? GetIntValue() : GetFloatValue())
			-
			(Rhs.GetType() == ESUDSValueType::Int ? Rhs.GetIntValue() : Rhs.GetFloatValue()));
	}
	FSUDSValue operator<(const FSUDSValue& Rhs) const
	{
		check(IsNumeric() && Rhs.IsNumeric());
		// result is boolean so no need to protect types
		return FSUDSValue(
			(Type == ESUDSValueType::Int ? GetIntValue() : GetFloatValue())
			<
			(Rhs.GetType() == ESUDSValueType::Int ? Rhs.GetIntValue() : Rhs.GetFloatValue()));
	}
	FSUDSValue operator==(const FSUDSValue& Rhs) const
	{
		if (IsNumeric())
		{
			check(Rhs.IsNumeric());
			if (GetType() == ESUDSValueType::Float || Rhs.GetType() == ESUDSValueType::Float)
			{
				// For floats, use tolerance
				return FSUDSValue(FMath::IsNearlyEqual(
					GetType() == ESUDSValueType::Int ? (float)GetIntValue() : GetFloatValue(),
					Rhs.GetType() == ESUDSValueType::Int ? (float)Rhs.GetIntValue() : Rhs.GetFloatValue()));
			}
			else
			{
				return FSUDSValue(GetIntValue() == Rhs.GetIntValue());
			}
		}
		else 
		{
			// no auto conversion here
			check(Type == Rhs.Type);
			switch (GetType())
			{
			case ESUDSValueType::Text:
				return FSUDSValue(GetTextValue().EqualTo(Rhs.GetTextValue()));
			case ESUDSValueType::Boolean:
				return FSUDSValue(GetBooleanValue() == Rhs.GetBooleanValue());
			case ESUDSValueType::Gender:
				return FSUDSValue(GetGenderValue() == Rhs.GetGenderValue());
			case ESUDSValueType::Variable:
				return FSUDSValue(GetVariableNameValue() == Rhs.GetVariableNameValue());
			default:
			case ESUDSValueType::Int:
			case ESUDSValueType::Float:
				// dealt with
				break;
			};
		}
		return FSUDSValue(false);
	}
	FSUDSValue operator<=(const FSUDSValue& Rhs) const
	{
		if ((*this < Rhs).GetBooleanValue())
			return FSUDSValue(true);
		return (*this == Rhs);
	}

	FSUDSValue operator>(const FSUDSValue& Rhs) const
	{
		return Rhs < *this;
	}

	FSUDSValue operator>=(const FSUDSValue& Rhs) const
	{
		return Rhs <= *this;
	}

	FSUDSValue operator!=(const FSUDSValue& Rhs) const
	{
		return !(*this == Rhs);
	}

	FSUDSValue operator&&(const FSUDSValue& Rhs) const
	{
		check(Type == ESUDSValueType::Boolean);
		check(Rhs.Type == ESUDSValueType::Boolean);
		return FSUDSValue(GetBooleanValue() && Rhs.GetBooleanValue());
	}

	FSUDSValue operator||(const FSUDSValue& Rhs) const
	{
		check(Type == ESUDSValueType::Boolean);
		check(Rhs.Type == ESUDSValueType::Boolean);
		return FSUDSValue(GetBooleanValue() || Rhs.GetBooleanValue());
	}

};

