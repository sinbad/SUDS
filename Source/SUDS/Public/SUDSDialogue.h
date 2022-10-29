#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSDialogue.generated.h"

class USUDSScriptNode;
class USUDSScript;
/**
 * A Dialogue is a runtime instance of a Script (the asset on which the dialogue is based)
 * An Dialogue instance involves specific parties taking the Roles, which may be Speakers in the Dialogue
 * or may be silent, providing or receiving data from the dialogue.
 * A dialogue instance has its own set of state, so you can invoke the same Script multiple times with different
 * things filling the Roles if you want. 
 */
UCLASS(BlueprintType)
class SUDS_API USUDSDialogue : public UObject
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	const USUDSScript* BaseScript;
	UPROPERTY()
	USUDSScriptNode* CurrentNode;

	FString CurrentTextID;
	FString CurrentSpeakerID;

	FText CurrentText;
	FText CurrentSpeakerDisplayName;

	void SetCurrentNode(USUDSScriptNode* Node);
public:
	USUDSDialogue();
	void Initialise(const USUDSScript* Script, FName StartLabel = NAME_None);
	

	/// Get the speech text for the current dialogue node
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FText& GetCurrentText() const;

	/// Get the ID of the current speaker
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FString& GetCurrentSpeakerID() const;

	/// Get the display name of the current speaker
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FText& GetCurrentSpeakerDisplayName() const;

	/// Get the number of choices available to advance the dialogue
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfChoices() const;

	/// Get the text associated with a choice
	UFUNCTION(BlueprintCallable)
	const FText& GetChoiceText(int Index) const;
	
	/**
	 * Continues the dialogue if (and ONLY if) there is only one path/choice out of the current node.
	 * @return True if the dialogue continues after this, false if the dialogue is now at an end.
	 */
	UFUNCTION(BlueprintCallable)
	bool Continue();
	
	/**
	 * Picks one of the available choices 
	 * If there's only 1 you can still call this with Index = 0, but also see Continue
	 * @param Index The index of the choice to make
	 * @return True if the dialogue continues, false if it has now reached the end.
	 */
	UFUNCTION(BlueprintCallable)
	bool Choose(int Index);

	/// Returns true if the dialogue has reached the end
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsEnded() const;

};
