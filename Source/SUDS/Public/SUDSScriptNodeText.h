// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeText.generated.h"

class UDialogueWave;

/**
* A node which contains speaker text 
*/
UCLASS(BlueprintType)
class SUDS_API USUDSScriptNodeText : public USUDSScriptNode
{
	GENERATED_BODY()

protected:
	/// Identifier of the speaker for text nodes
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category="SUDS")
	FString SpeakerID;
	/// Text, always references a string table. Parameters will not have been completed.
	/// Note: if you're using voiced dialogue, see the Wave property and its subtitle functionality
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category="SUDS")
	FText Text;
	/// DialogueWave asset link for voiced dialogue
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="SUDS")
	UDialogueWave* Wave;

	/// Convenience flag to let you know whether this text node MAY HAVE choices attached
	/// If false, there's only one way to proceed from here and no text associated with that
	/// If true, either there can be > 1 choice options, or a single choice with associated text (this can be when
	/// you have no choice but want text rather than just a continue button)
	/// Internally this also lets us know to look for the next choice node
	/// It's possible that where there are conditionals ahead, there are only choices on some of the paths.
	/// This flag is to let us know to look for choices, but if conditionals apply we may not find any using actual dialogue state.
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	bool bHasChoices = false;
	
	mutable bool bFormatExtracted = false; 
	mutable TArray<FName> ParameterNames;
	mutable FTextFormat TextFormat;

	void ExtractFormat() const;

public:
	const FString& GetSpeakerID() const { return SpeakerID; }
	const FText& GetText() const { return Text; }
	FString GetTextID() const;
	UDialogueWave* GetWave() const { return Wave; }
	/// Whether on one select path or another a choice was found
	/// Doesn't help if within a Gosub as call site may be anywhere
	bool MayHaveChoices() const { return bHasChoices; }

	void Init(const FString& SpeakerID, const FText& Text, int LineNo);
	void SetWave(UDialogueWave* InWave) { Wave = InWave; }
	const FTextFormat& GetTextFormat() const;
	const TArray<FName>& GetParameterNames() const;	
	bool HasParameters() const;

	void NotifyMayHaveChoices() { bHasChoices = true; }

};
