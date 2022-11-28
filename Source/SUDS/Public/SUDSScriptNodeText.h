#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeText.generated.h"

/**
* A node which contains speaker text 
*/
UCLASS(BlueprintType)
class SUDS_API USUDSScriptNodeText : public USUDSScriptNode
{
	GENERATED_BODY()

protected:
	/// Identifier of the speaker for text nodes
	UPROPERTY(BlueprintReadOnly)
	FString SpeakerID;
	/// Text, always references a string table. Parameters will not have been completed.
	UPROPERTY(BlueprintReadOnly)
	FText Text;

	/// Convenience flag to let you know whether this text node has any choices attached
	/// If false, there's only one way to proceed from here and no text associated with that
	/// If true, either there are > 1 choice options, or a single choice with associated text (this can be when
	/// you have no choice but want text rather than just a continue button)
	/// Internally this also lets us know to look for the next choice node
	UPROPERTY(BlueprintReadOnly)
	bool bHasChoices = false;
	
	mutable bool bFormatExtracted = false; 
	mutable TArray<FName> ParameterNames;
	mutable FTextFormat TextFormat;

	void ExtractFormat() const;

public:
	const FString& GetSpeakerID() const { return SpeakerID; }
	const FText& GetText() const { return Text; }
	FString GetTextID() const;
	bool HasChoices() const { return bHasChoices; }

	void Init(const FString& SpeakerID, const FText& Text, int LineNo);
	const FTextFormat& GetTextFormat() const;
	const TArray<FName>& GetParameterNames() const;	
	bool HasParameters() const;

	void NotifyHasChoices() { bHasChoices = true; }

};
