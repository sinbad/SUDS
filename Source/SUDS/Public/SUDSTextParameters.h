#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSTextParameters.generated.h"

/// Convenience object to hold named parameters for compatibility with Blueprints and C++
/// Would have been nice to have a USTRUCT but they can't have UFUNCTIONs
UCLASS(BlueprintType)
class SUDS_API USUDSTextParameters : public UObject
{
	GENERATED_BODY()
private:
	FFormatNamedArguments NamedArgs;

public:
	template <typename T>
	void SetParameter(const FString& Name, const T& Value) { NamedArgs.Add(Name, Value); }

	/// Set a text parameter
	UFUNCTION(BlueprintCallable)
	void SetTextParameter(FString Name, FText Value) { SetParameter(Name, Value); }
	/// Set an int parameter
	UFUNCTION(BlueprintCallable)
	void SetIntParameter(FString Name, int32 Value) { SetParameter(Name, Value); }
	/// Set an int64 parameter
	UFUNCTION(BlueprintCallable)
	void SetInt64Parameter(FString Name, int64 Value) { SetParameter(Name, Value); }
	/// Set a float parameter
	UFUNCTION(BlueprintCallable)
	void SetFloatParameter(FString Name, float Value) { SetParameter(Name, Value); }
	UFUNCTION(BlueprintCallable)
	/// Set a gender parameter
	void SetGenderParameter(FString Name, ETextGender Value) { SetParameter(Name, Value); }
	/// Set all parameters at once from a pre-prepared source
	UFUNCTION(BlueprintCallable)
	void SetAllParameters(const USUDSTextParameters* SourceArgs) { NamedArgs.Empty(); NamedArgs.Append(SourceArgs->NamedArgs); }

	void Empty() { NamedArgs.Empty(); }
	FText Format(const FTextFormat& FormatText) const
	{
		return FText::Format(FormatText, NamedArgs);
	}
};