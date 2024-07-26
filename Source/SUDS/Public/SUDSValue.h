// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "SUDSCommon.h"
#include "SUDSValue.generated.h"


UENUM(BlueprintType)
enum class ESUDSValueType : uint8
{
	Text = 0,
	Int = 1,
	Float = 2,
	Boolean = 3,
	Gender = 4,
	Name = 5,
	/// Access the value of another variable
	Variable = 10,

	Empty = 99
};
/// Struct which can hold any of the value types that SUDS needs to use, in a Blueprint friendly manner
/// For getting / setting these values from blueprints, see blueprint library functions SetSUDSValue<Type>() / GetSUDSValue<Type>()
/// For convenience these are wrapped in USUDSDialogue but in e.g. event callbacks they're not
USTRUCT(BlueprintType)
struct SUDS_API FSUDSValue
{
	GENERATED_BODY()
protected:
	ESUDSValueType Type;
	union
	{
		int32 IntValue;
		float FloatValue;
	};
	TOptional<FText> TextValue;
	// Used for variables and name values
	TOptional<FName> Name;
public:

	FSUDSValue() : Type(ESUDSValueType::Empty), IntValue(0), TextValue(FText::GetEmpty()) {}

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

	FSUDSValue(bool Value)
		: Type(ESUDSValueType::Boolean), IntValue(0)
	{
		IntValue = Value ? 1 : 0;
	}

	FSUDSValue(const FName& ReferencedName, bool bIsVariable)
	: Type(bIsVariable ? ESUDSValueType::Variable : ESUDSValueType::Name),
	  IntValue(0),
	  Name(ReferencedName)
	{
	}

	// Construct a default value of a given type
	explicit FSUDSValue(ESUDSValueType ValType)
		: Type(ValType), IntValue(0)
	{
	}

	/// Whether this value is empty, i.e. hasn't been set to anything
	FORCEINLINE bool IsEmpty() const
	{
		return Type == ESUDSValueType::Empty;
	}

	FORCEINLINE ESUDSValueType GetType() const
	{
		return Type;
	}

	FORCEINLINE int32 GetIntValue() const
	{
		// We don't warn for unset variables / uninitialised values, use the defaults
		if (!IsEmpty() && Type != ESUDSValueType::Int && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as int but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return IntValue;
	}

	FORCEINLINE float GetFloatValue() const
	{
		// We don't warn for unset variables, use the defaults
		if (!IsEmpty() && !IsNumeric() && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as float but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		if (Type == ESUDSValueType::Int)
		{
			// Allow int widening to float
			return GetIntValue();
		}
		return FloatValue;
	}

	FORCEINLINE const FText& GetTextValue() const
	{
		// We don't warn for unset variables, use the defaults
		if (!IsEmpty() && Type != ESUDSValueType::Text && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as text but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		if (TextValue.IsSet())
			return TextValue.GetValue();

		return FText::GetEmpty();
	}

	FORCEINLINE ETextGender GetGenderValue() const
	{
		// We don't warn for unset variables, use the defaults
		if (!IsEmpty() && Type != ESUDSValueType::Gender && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as float but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))
		
		return static_cast<ETextGender>(IntValue);
	}

	FORCEINLINE bool GetBooleanValue() const
	{
		// We don't warn for unset variables, use the defaults
		if (!IsEmpty() && Type != ESUDSValueType::Boolean && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as boolean but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		return IntValue != 0;
	}

	FORCEINLINE FName GetNameValue() const
	{
		// We don't warn for unset variables, use the defaults
		if (!IsEmpty() && Type != ESUDSValueType::Name && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as Name but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		if (Name.IsSet())
			return Name.GetValue();

		return NAME_None;
	}

	FORCEINLINE FName GetVariableNameValue() const
	{
		if (!IsEmpty() && Type != ESUDSValueType::Variable)
			UE_LOG(LogSUDS, Warning, TEXT("Getting value as variable name but was type %s"), *StaticEnum<ESUDSValueType>()->GetValueAsString(Type))

		if (Name.IsSet())
			return Name.GetValue();

		return NAME_None;
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
		case ESUDSValueType::Empty:
		case ESUDSValueType::Name:
		case ESUDSValueType::Variable:
			return FFormatArgumentValue();
		}
	}

	void SetValue(const FSUDSValue& RValue)
	{
		*this = RValue;
	}

	/// Not operation, technically only valid on booleans, but not enforced so as to allow unset vars & conversions
	FSUDSValue operator!() const
	{
		return FSUDSValue(!GetBooleanValue());
	}

	FSUDSValue operator*(const FSUDSValue& Rhs) const
	{
		// We don't force types to be numeric here so that we can safely use unset variables
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() * Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue())
			*
			(Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue()));
	}
	FSUDSValue operator/(const FSUDSValue& Rhs) const
	{
		// We don't force types to be numeric here so that we can safely use unset variables
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() / Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue())
			/
			(Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue()));
	}
	FSUDSValue operator%(const FSUDSValue& Rhs) const
	{
		// We don't force types to be numeric here so that we can safely use unset variables
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() % Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		float lval = (Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue());
		float rval = (Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue());
		// Prevent NaN and errors reported when executing FMath::Fmod with invalid values.
		return FSUDSValue(rval != 0 ? FMath::Fmod(lval, rval) : 0.0f);
	}
	FSUDSValue operator+(const FSUDSValue& Rhs) const
	{
		// We don't force types to be numeric here so that we can safely use unset variables
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() + Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue())
			+
			(Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue()));
	}
	FSUDSValue operator-(const FSUDSValue& Rhs) const
	{
		// We don't force types to be numeric here so that we can safely use unset variables
		// If both int, keep int
		if (Type == ESUDSValueType::Int && Rhs.Type == Type)
		{
			return FSUDSValue(GetIntValue() - Rhs.GetIntValue());
		}
		// Otherwise we'll let the type widening handle mixed types for us (ternary will always resolve to float)
		return FSUDSValue(
			(Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue())
			-
			(Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue()));
	}
	FSUDSValue operator<(const FSUDSValue& Rhs) const
	{
		// Don't check types here. We'll fall back on int comparisons which is important for cases where
		// a variable hasn't been set
		// result is boolean so no need to protect types
		return FSUDSValue(
			(Type == ESUDSValueType::Float ? GetFloatValue() : GetIntValue())
			<
			(Rhs.GetType() == ESUDSValueType::Float ? Rhs.GetFloatValue() : Rhs.GetIntValue()));
		
	}
	FSUDSValue operator==(const FSUDSValue& Rhs) const
	{
		if (IsNumeric() || Rhs.IsNumeric())
		{
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
			// however, tolerate unresolved variables, they will return initial values
			if (Type != Rhs.Type && Type != ESUDSValueType::Variable && Rhs.Type != ESUDSValueType::Variable)
			{
				return false;
			}

			// Compare using type from whichever one isn't a variable (get values on unset variables will return defaults)
			ESUDSValueType UseType = IsVariable() ? Rhs.GetType() : GetType();
			switch (UseType)
			{
			case ESUDSValueType::Text:
				return FSUDSValue(GetTextValue().EqualTo(Rhs.GetTextValue()));
			case ESUDSValueType::Boolean:
				return FSUDSValue(GetBooleanValue() == Rhs.GetBooleanValue());
			case ESUDSValueType::Gender:
				return FSUDSValue(GetGenderValue() == Rhs.GetGenderValue());
			case ESUDSValueType::Variable:
				return FSUDSValue(GetVariableNameValue() == Rhs.GetVariableNameValue());
			case ESUDSValueType::Name:
				return FSUDSValue(GetNameValue() == Rhs.GetNameValue());
				// deal with int/float again here, this mops up cases where one side is an unset variable
			case ESUDSValueType::Int:
				return FSUDSValue(GetIntValue() == Rhs.GetIntValue());
			case ESUDSValueType::Float:
				return FSUDSValue(GetFloatValue() == Rhs.GetFloatValue());
			default:
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
		// We always let unset variables degrade to false
		check(Type == ESUDSValueType::Boolean || Type == ESUDSValueType::Variable);
		check(Rhs.Type == ESUDSValueType::Boolean || Rhs.Type == ESUDSValueType::Variable);
		return FSUDSValue(GetBooleanValue() && Rhs.GetBooleanValue());
	}

	FSUDSValue operator||(const FSUDSValue& Rhs) const
	{
		// We always let unset variables degrade to false
		check(Type == ESUDSValueType::Boolean || Type == ESUDSValueType::Variable);
		check(Rhs.Type == ESUDSValueType::Boolean || Rhs.Type == ESUDSValueType::Variable);
		return FSUDSValue(GetBooleanValue() || Rhs.GetBooleanValue());
	}

	SUDS_API friend FArchive& operator<<(FArchive& Ar, FSUDSValue& Value);
	SUDS_API friend void operator<<(FStructuredArchive::FSlot Slot, FSUDSValue& Value);
	bool Serialize(FArchive& Ar)
	{
		Ar << *this;
		return true;
	}
	bool Serialize(FStructuredArchive::FSlot Slot)
	{
		Slot << *this;
		return true;
	}
	
	FString ToString() const;

	bool ExportTextItem(FString& ValueStr, FSUDSValue const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
};
template<>
struct TStructOpsTypeTraits<FSUDSValue> : public TStructOpsTypeTraitsBase2<FSUDSValue>
{
	enum
	{
		WithSerializer = true,
		WithExportTextItem = true
	};
};



