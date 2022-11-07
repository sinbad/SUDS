#pragma once

#include "CoreMinimal.h"
#include "SUDSTextParameters.generated.h"

/// FFormatNamedArguments can't be used directly in Blueprints because it's not BlueprintType
/// USTRUCT to be GC friendly, values updated via SUDSLibrary BPL (see SetDialogueTextParameter etc)


/// Convenience object to hold named parameters for compatibility with Blueprints and C++
USTRUCT(BlueprintType)
struct SUDS_API FSUDSTextParameters
{
	GENERATED_BODY()
private:
	FFormatNamedArguments NamedArgs;

public:
	template <typename T>
	void SetParameter(const FString& Name, const T& Value) { NamedArgs.Add(Name, Value); }

	void Empty() { NamedArgs.Empty(); }
	FText Format(const FTextFormat& FormatText) const
	{
		return FText::Format(FormatText, NamedArgs);
	}
};