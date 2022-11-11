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
	Gender
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

	
	
};
